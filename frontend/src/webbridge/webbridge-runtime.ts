/**
 * WebBridge Runtime - Minimal Implementation
 * 
 * Provides the JavaScript runtime for C++ class bindings.
 * Called by C++ init() scripts after polling for availability.
 */

// Object registry: objectId -> object instance
const objects: Record<string, any> = {};

// =============================================================================
// C++ -> JS Handlers (called by C++ via webview eval)
// =============================================================================

(window as any).__webbridge_notify = (objectId: string, propName: string, value: any) => {
    objects[objectId]?.[propName]?._notify?.(value);
};

(window as any).__webbridge_emit = (objectId: string, eventName: string, ...args: any[]) => {
    objects[objectId]?.[eventName]?._dispatch?.(...args);
};

// =============================================================================
// Property: Svelte-compatible store
// =============================================================================

function createProperty<T>(objectId: string, className: string, propName: string) {
    const subscribers = new Set<(value: T) => void>();
    let currentValue: T | undefined;
    let loaded = false;

    return {
        subscribe(callback: (value: T) => void) {
            subscribers.add(callback);
            if (loaded) {
                callback(currentValue as T);
            } else {
                (window as any)[`__get_${className}_${propName}`](objectId).then((v: T) => {
                    currentValue = v;
                    loaded = true;
                    callback(v);
                });
            }
            return () => subscribers.delete(callback);
        },
        async get(): Promise<T> {
            if (!loaded) {
                currentValue = await (window as any)[`__get_${className}_${propName}`](objectId);
                loaded = true;
            }
            return currentValue as T;
        },
        _notify(value: T) {
            currentValue = value;
            loaded = true;
            subscribers.forEach(fn => fn(value));
        }
    };
}

// =============================================================================
// Event: on/once pattern
// =============================================================================

function createEvent<TArgs extends any[]>(objectId: string, className: string, eventName: string) {
    const listeners: Array<{ fn: (...args: TArgs) => void; once: boolean }> = [];

    return {
        on(callback: (...args: TArgs) => void) {
            listeners.push({ fn: callback, once: false });
            return () => {
                const idx = listeners.findIndex(l => l.fn === callback);
                if (idx !== -1) listeners.splice(idx, 1);
            };
        },
        once(callback: (...args: TArgs) => void) {
            listeners.push({ fn: callback, once: true });
        },
        _dispatch(...args: TArgs) {
            for (let i = listeners.length - 1; i >= 0; i--) {
                listeners[i].fn(...args);
                if (listeners[i].once) listeners.splice(i, 1);
            }
        }
    };
}

// =============================================================================
// Class Factory
// =============================================================================

interface ClassConfig {
    className: string;
    properties: string[];
    events: string[];
    methods: string[];
    instanceConstants: string[];
    staticConstants: Record<string, any>;
}

function createClass(config: ClassConfig) {
    const { className, properties, events, methods, instanceConstants, staticConstants } = config;

    const factory: any = {
        async create(...args: any[]) {
            const objectId: string = await (window as any)[`__create_${className}`](...args);

            const obj: any = {
                __id: objectId,
                get handle() { return this.__id; },
                destroy() {
                    delete objects[objectId];
                    (window as any).__webbridge_destroy?.(objectId);
                }
            };

            for (const p of properties) obj[p] = createProperty(objectId, className, p);
            for (const e of events) obj[e] = createEvent(objectId, className, e);
            for (const m of methods) {
                obj[m] = (...a: any[]) => (window as any)[`__${className}_${m}`](objectId, ...a);
            }
            for (const c of instanceConstants) {
                obj[c] = await (window as any)[`__get_${className}_${c}`](objectId);
            }
            // Copy static constants to instance for convenience
            for (const [key, value] of Object.entries(staticConstants)) {
                obj[key] = value;
            }

            objects[objectId] = obj;
            return obj;
        }
    };

    // Assign static constants to factory (class level)
    for (const [key, value] of Object.entries(staticConstants)) {
        factory[key] = value;
    }

    return factory;
}

// =============================================================================
// Export Runtime to window (for C++ init scripts)
// =============================================================================

(window as any).WebbridgeRuntime = {
    createClass
};

console.log('[WebBridge] Runtime loaded');
