/**
 * WebBridge Runtime Library
 * 
 * This library provides the JavaScript runtime for WebBridge.
 * It is loaded once and shared by all registered C++ classes.
 * 
 * @module webbridge-runtime
 */

import type {
    WebbridgeProperty,
    WebbridgeEvent,
    WebbridgeClassConfig,
    WebbridgeObject
} from './webbridge-types';

// =============================================================================
// WebbridgeProperty Implementation
// =============================================================================

/**
 * Creates a Svelte-compatible store for a C++ property.
 * Supports subscribe(), get(), and receives updates from C++ via _notify().
 */
export function createProperty<T>(
    objectId: string,
    className: string,
    propName: string
): WebbridgeProperty<T> {
    const subscribers = new Set<(value: T) => void>();
    let currentValue: T | undefined = undefined;
    let loaded = false;

    const ensureValue = async (): Promise<T> => {
        if (!loaded) {
            const getter = (window as any)[`__get_${className}_${propName}`];
            if (!getter) {
                throw new Error(`Property getter not found: ${className}.${propName}`);
            }
            currentValue = await getter(objectId);
            loaded = true;
        }
        return currentValue as T;
    };

    return {
        subscribe(callback: (value: T) => void) {
            subscribers.add(callback);
            
            // Initial call with current or fetched value
            ensureValue().then(callback);

            // Return unsubscribe function
            return () => {
                subscribers.delete(callback);
            };
        },

        async get(): Promise<T> {
            return ensureValue();
        },

        // Internal: Called by C++ via __webbridge_notify
        _notify(value: T) {
            currentValue = value;
            loaded = true;
            subscribers.forEach(fn => {
                try {
                    fn(value);
                } catch (e) {
                    console.error(`[WebBridge] Error in property subscriber for ${className}.${propName}:`, e);
                }
            });
        }
    };
}

// =============================================================================
// WebbridgeEvent Implementation
// =============================================================================

/**
 * Creates an event emitter for C++ events.
 * Supports on(), once(), and receives events from C++ via _dispatch().
 */
export function createEvent<TArgs extends any[]>(
    objectId: string,
    className: string,
    eventName: string
): WebbridgeEvent<TArgs> {
    const listeners: Array<{ fn: (...args: TArgs) => void; once: boolean }> = [];

    return {
        on(callback: (...args: TArgs) => void) {
            listeners.push({ fn: callback, once: false });
            
            // Return unsubscribe function
            return () => {
                const idx = listeners.findIndex(l => l.fn === callback);
                if (idx !== -1) {
                    listeners.splice(idx, 1);
                }
            };
        },

        once(callback: (...args: TArgs) => void) {
            listeners.push({ fn: callback, once: true });
        },

        // Internal: Called by C++ via __webbridge_emit
        _dispatch(...args: TArgs) {
            // Filter removes once-listeners after calling
            for (let i = listeners.length - 1; i >= 0; i--) {
                const listener = listeners[i];
                try {
                    listener.fn(...args);
                } catch (e) {
                    console.error(`[WebBridge] Error in event listener for ${className}.${eventName}:`, e);
                }
                if (listener.once) {
                    listeners.splice(i, 1);
                }
            }
        },

        // For debugging
        get listenerCount() {
            return listeners.length;
        }
    };
}

// =============================================================================
// WebbridgeRuntime Core
// =============================================================================

export class WebbridgeRuntime {
    // Global object registry: objectId -> object instance
    static objects: Record<string, WebbridgeObject> = {};

    // Track if runtime is initialized
    static initialized = false;

    /**
     * Initialize the runtime. Called automatically on import.
     */
    static init() {
        if (this.initialized) {
            return;
        }

        // Setup global registry
        (window as any).__webbridge_objects = this.objects;

        // Setup property notification handler (C++ -> JS)
        (window as any).__webbridge_notify = (
            objectId: string,
            propName: string,
            value: any
        ) => {
            const obj = this.objects[objectId];
            if (!obj) {
                console.warn(`[WebBridge] Object ${objectId} not found for property notification: ${propName}`);
                return;
            }

            const prop = (obj as any)[propName];
            if (prop?._notify) {
                prop._notify(value);
            } else {
                console.warn(`[WebBridge] Property ${propName} not found on object ${objectId}`);
            }
        };

        // Setup event emission handler (C++ -> JS)
        (window as any).__webbridge_emit = (
            objectId: string,
            eventName: string,
            ...args: any[]
        ) => {
            const obj = this.objects[objectId];
            if (!obj) {
                console.warn(`[WebBridge] Object ${objectId} not found for event: ${eventName}`);
                return;
            }

            const event = (obj as any)[eventName];
            if (event?._dispatch) {
                event._dispatch(...args);
            } else {
                console.warn(`[WebBridge] Event ${eventName} not found on object ${objectId}`);
            }
        };

        this.initialized = true;
        console.log('[WebBridge] Runtime initialized');
    }

    /**
     * Register an object in the global registry.
     */
    static registerObject(objectId: string, obj: WebbridgeObject) {
        this.objects[objectId] = obj;
    }

    /**
     * Remove an object from the global registry.
     */
    static unregisterObject(objectId: string) {
        delete this.objects[objectId];
    }

    /**
     * Create a sync method wrapper.
     */
    static createSyncMethod<TArgs extends any[], TReturn>(
        className: string,
        methodName: string
    ): (objectId: string, ...args: TArgs) => Promise<TReturn> {
        return async (objectId: string, ...args: TArgs): Promise<TReturn> => {
            const fn = (window as any)[`__${className}_${methodName}`];
            if (!fn) {
                throw new Error(`Method not found: ${className}.${methodName}`);
            }
            return await fn(objectId, ...args);
        };
    }

    /**
     * Create an async method wrapper.
     */
    static createAsyncMethod<TArgs extends any[], TReturn>(
        className: string,
        methodName: string
    ): (objectId: string, ...args: TArgs) => Promise<TReturn> {
        return async (objectId: string, ...args: TArgs): Promise<TReturn> => {
            const fn = (window as any)[`__${className}_${methodName}`];
            if (!fn) {
                throw new Error(`Async method not found: ${className}.${methodName}`);
            }
            return await fn(objectId, ...args);
        };
    }

    /**
     * Create a WebBridge class from configuration.
     * This is the main factory function used by generated code.
     */
    static createClass<T extends WebbridgeObject>(
        config: WebbridgeClassConfig
    ): { create: (...args: any[]) => Promise<T> } & Record<string, any> {
        const {
            className,
            properties = [],
            events = [],
            syncMethods = [],
            asyncMethods = [],
            instanceConstants = [],
            staticConstants = {}
        } = config;

        // FinalizationRegistry for GC safety net
        const pointers = new FinalizationRegistry<string>((handle) => {
            console.warn(`[WebBridge] ${className} was garbage collected without destroy() - cleaning up`);
            WebbridgeRuntime.unregisterObject(handle);
            const destroyFn = (window as any).__WebBridge_destroy;
            if (destroyFn) {
                destroyFn(handle);
            }
        });

        // The class constructor/factory
        const WebbridgeClass = {
            async create(...args: any[]): Promise<T> {
                const createFn = (window as any)[`__create_${className}`];
                if (!createFn) {
                    throw new Error(`Constructor not found: __create_${className}`);
                }

                const objectId: string = await createFn(...args);

                // Create object instance
                const obj: any = {
                    __id: objectId,
                    __type: className,
                    __destroyed: false,

                    get handle() {
                        if (this.__destroyed) {
                            throw new Error(`${className} object already destroyed`);
                        }
                        return this.__id;
                    },

                    destroy() {
                        if (this.__destroyed) return;
                        this.__destroyed = true;

                        WebbridgeRuntime.unregisterObject(objectId);
                        pointers.unregister(this);

                        const destroyFn = (window as any).__WebBridge_destroy;
                        if (destroyFn) {
                            destroyFn(objectId);
                        }
                    }
                };

                // Setup properties
                for (const propName of properties) {
                    obj[propName] = createProperty(objectId, className, propName);
                }

                // Setup events
                for (const eventName of events) {
                    obj[eventName] = createEvent(objectId, className, eventName);
                }

                // Setup sync methods
                for (const methodName of syncMethods) {
                    const method = WebbridgeRuntime.createSyncMethod(className, methodName);
                    obj[methodName] = (...args: any[]) => method(obj.handle, ...args);
                }

                // Setup async methods
                for (const methodName of asyncMethods) {
                    const method = WebbridgeRuntime.createAsyncMethod(className, methodName);
                    obj[methodName] = (...args: any[]) => method(obj.handle, ...args);
                }

                // Fetch instance constants
                for (const constName of instanceConstants) {
                    const getter = (window as any)[`__get_${className}_${constName}`];
                    if (getter) {
                        obj[constName] = await getter(objectId);
                    }
                }

                // Copy static constants to instance (for convenience)
                for (const constName of Object.keys(staticConstants)) {
                    obj[constName] = staticConstants[constName];
                }

                // Register in global registry
                WebbridgeRuntime.registerObject(objectId, obj);

                // Register with FinalizationRegistry as safety net
                pointers.register(obj, objectId, obj);

                return obj as T;
            }
        };

        // Add static constants to the class itself
        for (const [name, value] of Object.entries(staticConstants)) {
            Object.defineProperty(WebbridgeClass, name, {
                value,
                writable: false,
                enumerable: true,
                configurable: false
            });
        }

        return WebbridgeClass as any;
    }

    /**
     * Create a published object wrapper (for objects created from C++).
     */
    static createPublishedObject<T extends WebbridgeObject>(
        className: string,
        objectId: string,
        config: Omit<WebbridgeClassConfig, 'className'>
    ): T {
        const {
            properties = [],
            events = [],
            syncMethods = [],
            asyncMethods = [],
            instanceConstants = [],
            staticConstants = {}
        } = config;

        const obj: any = {
            __id: objectId,
            __type: className,

            get handle() {
                return this.__id;
            }
        };

        // Setup properties
        for (const propName of properties) {
            obj[propName] = createProperty(objectId, className, propName);
        }

        // Setup events
        for (const eventName of events) {
            obj[eventName] = createEvent(objectId, className, eventName);
        }

        // Setup sync methods
        for (const methodName of syncMethods) {
            const method = WebbridgeRuntime.createSyncMethod(className, methodName);
            obj[methodName] = (...args: any[]) => method(obj.handle, ...args);
        }

        // Setup async methods
        for (const methodName of asyncMethods) {
            const method = WebbridgeRuntime.createAsyncMethod(className, methodName);
            obj[methodName] = (...args: any[]) => method(obj.handle, ...args);
        }

        // Copy static constants to instance
        for (const constName of Object.keys(staticConstants)) {
            obj[constName] = staticConstants[constName];
        }

        // Register in global registry
        WebbridgeRuntime.registerObject(objectId, obj);

        return obj as T;
    }
}

// Auto-initialize on import
WebbridgeRuntime.init();

// Export for window access
(window as any).WebbridgeRuntime = WebbridgeRuntime;
