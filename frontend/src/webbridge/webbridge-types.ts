/**
 * WebBridge Type Definitions
 * 
 * This file contains TypeScript type definitions for the WebBridge runtime.
 * 
 * @module webbridge-types
 */

// =============================================================================
// Property Types
// =============================================================================

/**
 * A Svelte-compatible store for a C++ property.
 * Can be used directly in Svelte templates with $property syntax.
 */
export interface WebbridgeProperty<T> {
    /**
     * Subscribe to property changes.
     * Compatible with Svelte's store contract.
     * 
     * @param callback Called with current value immediately and on each change
     * @returns Unsubscribe function
     */
    subscribe(callback: (value: T) => void): () => void;

    /**
     * Get the current value asynchronously.
     * Fetches from C++ if not already loaded.
     */
    get(): Promise<T>;

    /**
     * Internal: Called by C++ to notify of value changes.
     * @internal
     */
    _notify(value: T): void;
}

// =============================================================================
// Event Types
// =============================================================================

/**
 * An event emitter for C++ events.
 */
export interface WebbridgeEvent<TArgs extends any[] = any[]> {
    /**
     * Subscribe to this event.
     * 
     * @param callback Called each time the event is emitted
     * @returns Unsubscribe function
     */
    on(callback: (...args: TArgs) => void): () => void;

    /**
     * Subscribe to this event for a single emission.
     * 
     * @param callback Called once when the event is emitted
     */
    once(callback: (...args: TArgs) => void): void;

    /**
     * Internal: Called by C++ to dispatch the event.
     * @internal
     */
    _dispatch(...args: TArgs): void;

    /**
     * Get the number of active listeners (for debugging).
     */
    readonly listenerCount: number;
}

// =============================================================================
// Object Types
// =============================================================================

/**
 * Base interface for all WebBridge objects.
 */
export interface WebbridgeObject {
    /**
     * The unique handle/ID for this object.
     */
    readonly handle: string;

    /**
     * Destroy this object and release C++ resources.
     * After calling destroy(), the object should not be used.
     */
    destroy(): void;
}

// =============================================================================
// Configuration Types
// =============================================================================

/**
 * Configuration for creating a WebBridge class.
 */
export interface WebbridgeClassConfig {
    /**
     * The name of the C++ class.
     */
    className: string;

    /**
     * Names of properties (Svelte stores).
     */
    properties?: string[];

    /**
     * Names of events.
     */
    events?: string[];

    /**
     * Names of synchronous methods.
     */
    syncMethods?: string[];

    /**
     * Names of asynchronous methods.
     */
    asyncMethods?: string[];

    /**
     * Names of instance constants (read-only, fetched once).
     */
    instanceConstants?: string[];

    /**
     * Static constants (name -> value).
     */
    staticConstants?: Record<string, any>;
}

// =============================================================================
// Published Object Types
// =============================================================================

/**
 * Configuration for a published object message from C++.
 */
export interface WebbridgePublishConfig {
    className: string;
    varName: string;
    objectId: string;
    properties: string[];
    events: string[];
    syncMethods: string[];
    asyncMethods: string[];
    instanceConstants: string[];
    staticConstants: Record<string, any>;
}

// =============================================================================
// Window Extensions
// =============================================================================

/**
 * Extend the Window interface with WebBridge globals.
 */
declare global {
    interface Window {
        /**
         * Global registry of all WebBridge objects.
         */
        __webbridge_objects: Record<string, WebbridgeObject>;

        /**
         * Called by C++ to notify property changes.
         */
        __webbridge_notify: (objectId: string, propName: string, value: any) => void;

        /**
         * Called by C++ to emit events.
         */
        __webbridge_emit: (objectId: string, eventName: string, ...args: any[]) => void;

        /**
         * The WebBridge runtime instance.
         */
        WebbridgeRuntime: typeof import('./webbridge-runtime').WebbridgeRuntime;
    }
}

export {};
