/**
 * WebBridge Module - Entry point for frontend library
 */

// Runtime must be imported to register handlers on window
import './webbridge-runtime';

export type {
    WebbridgeProperty,
    WebbridgeEvent,
    WebbridgeObject
} from './webbridge-types';
