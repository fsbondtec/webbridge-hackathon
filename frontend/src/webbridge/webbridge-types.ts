/**
 * WebBridge Type Definitions
 */

// Svelte-compatible store for C++ properties
export interface WebbridgeProperty<T> {
    subscribe(callback: (value: T) => void): () => void;
    get(): Promise<T>;
}

// Event emitter for C++ events
export interface WebbridgeEvent<TArgs extends any[] = any[]> {
    on(callback: (...args: TArgs) => void): () => void;
    once(callback: (...args: TArgs) => void): void;
}

// Base interface for WebBridge objects
export interface WebbridgeObject {
    readonly handle: string;
    destroy(): void;
}
