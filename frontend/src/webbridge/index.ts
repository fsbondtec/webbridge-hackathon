/**
 * WebBridge Module
 * 
 * Main entry point for the WebBridge frontend library.
 * 
 * @example
 * ```typescript
 * import { WebbridgeRuntime, createProperty, createEvent } from './webbridge';
 * 
 * // The runtime is auto-initialized on import
 * // Classes are registered via C++ using WebbridgeRuntime.createClass()
 * ```
 */

export { WebbridgeRuntime, createProperty, createEvent } from './webbridge-runtime';

export type {
    WebbridgeProperty,
    WebbridgeEvent,
    WebbridgeObject,
    WebbridgeClassConfig,
    WebbridgePublishConfig
} from './webbridge-types';
