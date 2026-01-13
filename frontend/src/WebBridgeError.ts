// Auto-generated TypeScript definitions for WebBridgeError
// WebBridge Runtime Error Class

/**
 * Custom Error class for C++ exceptions thrown through WebBridge
 * Extends the standard Error class with additional metadata
 * 
 * This class is injected by the C++ runtime and available on window.WebBridgeError
 */
export interface WebBridgeError extends Error {
  readonly code: number;
  readonly origin: 'javascript' | 'cpp' | 'unknown';
  readonly cppFunction: string | null;
  readonly cppName: string | null;
}

/**
 * WebBridgeError constructor interface
 */
export interface WebBridgeErrorConstructor {
  new (rawError: any): WebBridgeError;
  readonly prototype: WebBridgeError;
  
  // Static error code constants
  readonly JSON_PARSE_ERROR: 4001;
  readonly JSON_TYPE_ERROR: 4002;
  readonly JSON_ACCESS_ERROR: 4003;
  readonly INVALID_ARGUMENT: 4004;
  readonly OBJECT_NOT_FOUND: 4005;
  readonly RUNTIME_ERROR: 5000;
  readonly NETWORK_ERROR: 5001;
  readonly FILE_ERROR: 5002;
  readonly TIMEOUT_ERROR: 5003;
  readonly PERMISSION_ERROR: 5004;
  readonly CUSTOM_ERROR: 5500;
}

// Declare the global WebBridgeError class (injected by C++ runtime)
declare global {
  interface Window {
    WebBridgeError: WebBridgeErrorConstructor;
  }
}

// Export the class from the global window object
export const WebBridgeError = window.WebBridgeError;
