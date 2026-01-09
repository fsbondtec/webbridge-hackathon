<script lang="ts">
  import { onMount, onDestroy } from 'svelte';
  import { MyObject } from './MyObject';
  import { TestObject } from './TestObject';
  import { TestObject8 } from './TestObject8';
  import bildUrl from './assets/bild.jpg';

  // State
  let obj: MyObject | null = null;
  let logs: string[] = [];

  // Store subscriptions
  $: aBool = obj?.aBool;
  $: strProp = obj?.strProp;
  $: counter = obj?.counter;
  $: numbers = obj?.numbers;
  $: status = obj?.status;
  $: pod = obj?.pod;

  function log(msg: string, type: 'info' | 'success' | 'error' = 'info') {
    const timestamp = new Date().toLocaleTimeString();
    const prefix = type === 'error' ? '❌' : type === 'success' ? '✅' : 'ℹ️';
    logs = [...logs, `[${timestamp}] ${prefix} ${msg}`];
    console.log(msg);
  }

  async function createObject() {
    try {
      log('Creating object...', 'info');

      log(`Static constant appversion: ${MyObject.appversion}`, 'info');
      const newObj = await MyObject.create('fab');
      log(`Object created: ${newObj.handle}`, 'success');
      newObj.aEvent.on((intVal, boolVal) => {
        log(`🔔 Event received: int=${intVal}, bool=${boolVal}`);
      });

      // Log constants (instance and static)
      log(`Instance constant aversion: ${newObj.version}`, 'info');
      log(`Instance constant CPP_VERSION: ${MyObject.CPP_VERSION}`, 'info');

      obj = newObj;
    } catch (error) {
      log(`Failed to create object: ${error}`, 'error');
    }
  }

  function destroyObject() {
    if (!obj) {
      log('No object to destroy!', 'error');
      return;
    }

    try {
      const objToDestroy = obj;
      obj = null;
      objToDestroy.destroy();
      log('Object destroyed', 'success');
    } catch (error) {
      log(`Failed to destroy object: ${error}`, 'error');
    }
  }

  onDestroy(() => {
    if (obj) {
      obj.destroy();
    }
  });

  async function callBar() {
    if (!obj) {
      log('No object!', 'error');
      return;
    }

    try {
      log('Calling bar()...', 'info');
      const result = await obj.bar();
      log(`bar() returned: ${result}`, 'success');
    } catch (error) {
      log(`bar() failed: ${error}`, 'error');
    }
  }

  async function callFoo() {
    if (!obj) {
      log('No object!', 'error');
      return;
    }

    try {
      log('Calling foo() (async)...', 'info');
      await obj.foo('Hello from Svelte! 🎉');
      log('foo() completed', 'success');
    } catch (error) {
      log(`foo() failed: ${error}`, 'error');
    }
  }

  async function callFile() {
    if (!obj) {
      log('No object!', 'error');
      return;
    }

    try {
      log('Opening file dialog...', 'info');
      await obj.file();
      log('File dialog completed', 'success');
    } catch (error) {
      log(`file() failed: ${error}`, 'error');
    }
  }

  async function callTestVectors() {
    if (!obj) {
      log('No object!', 'error');
      return;
    }

    try {
      log('Calling testVectors()...', 'info');
      await obj.testVectors();
      log('testVectors() completed - check properties above!', 'success');
    } catch (error) {
      log(`testVectors() failed: ${error}`, 'error');
    }
  }

  async function callThrowError() {
    if (!obj) {
      log('No object!', 'error');
      return;
    }
    
    try {
      log('Calling throwError()...', 'info');
      await obj.throwError();
      log('throwError() completed (should not happen!)', 'success');
    } catch (error) {
      log(`throwError() failed (expected): ${JSON.stringify(error)}`, 'error');
    }
  }

  async function callMultiParamTest() {
    if (!obj) {
      log('No object!', 'error');
      return;
    }
    
    await obj.bar();
    
    try {
      log('Calling multiParamTest() with 6 different parameter types...', 'info');
      const result = await obj.multiParamTest(
        999,                           // int
        true,                          // bool
        'Test String from JS',         // string
        [10, 20, 30, 40, 50],         // vector<int>
        MyObject.Status.Running,      // Status enum
        { a: 777, b: 888888888 }      // Pod struct
      );
      log(`multiParamTest() returned: ${result}`, 'success');
      log('✨ Check properties above - they should all be updated!', 'info');
    } catch (error) {
      log(`multiParamTest() failed: ${JSON.stringify(error)}`, 'error');
    }
  }

  function clearLog() {
    logs = [];
    log('Log cleared', 'info');
  }

  // ============ BENCHMARK FUNCTIONALITY ============
  
  interface BenchmarkStats {
    min: number;
    max: number;
    avg: number;
    stdev: number;
    total: number;
  }

  function calculateStats(values: number[]): BenchmarkStats {
    if (values.length === 0) {
      return { min: 0, max: 0, avg: 0, stdev: 0, total: 0 };
    }

    const min = Math.min(...values);
    const max = Math.max(...values);
    const total = values.reduce((a, b) => a + b, 0);
    const avg = total / values.length;
    
    // Standard deviation
    const variance = values.reduce((sum, val) => sum + Math.pow(val - avg, 2), 0) / values.length;
    const stdev = Math.sqrt(variance);

    return { min, max, avg, stdev, total };
  }

  async function testTestObject8() {
    try {
      log('=== Testing TestObject8 ===', 'info');
      
      // Create TestObject8
      log('Creating TestObject8...', 'info');
      const testObj8 = await TestObject8.create();
      log(`TestObject8 created: ${testObj8.handle}`, 'success');
      // testObj8.handle = "futarschbeidl";
      
      // Log constants
      log(`CONST1: ${TestObject8.CONST1}, CONST2: ${TestObject8.CONST2}`, 'info');

      // Read initial properties via subscription
      let prop1Val: number | undefined;
      let prop2Val: string | undefined;
      const unsubProp1 = testObj8.prop1.subscribe(v => prop1Val = v);
      const unsubProp2 = testObj8.prop2.subscribe(v => prop2Val = v);
      // Wait a bit for values to be fetched
      await new Promise(resolve => setTimeout(resolve, 100));
      log(`Initial prop1: ${prop1Val}, prop2: ${prop2Val}`, 'info');
      unsubProp1.unsubscribe();
      unsubProp2.unsubscribe();

      // Call method1 (sync)
      log('Calling method1(42)...', 'info');
      const result1 = await testObj8.method1(42);
      log(`method1(42) returned: ${result1}`, 'success');

      // Call method2 (async)
      log('Calling method2("Hello")...', 'info');
      const result2 = await testObj8.method2("Hello");
      log(`method2("Hello") returned: ${result2}`, 'success');

      // Cleanup
      log('Destroying TestObject8...', 'info');
      testObj8.destroy();
      log('TestObject8 destroyed', 'success');
      log('=== TestObject8 test complete ===', 'success');

    } catch (error) {
      log(`TestObject8 test failed: ${error}`, 'error');
    }
  }

  async function runJsonBenchmark() {
    try {
      log('=== STARTING JSON BENCHMARK ===', 'info');
      
      // Create TestObject
      log('Creating TestObject...', 'info');
      const testObj = await TestObject.create();

      log(`TestObject created: ${testObj.handle}`, 'success');

      log('Running JSON deserialization benchmark (1000 iterations in C++)...', 'info');
      const start = performance.now();
      const cppDuration = await testObj.jsonBench();
      const jsOverhead = performance.now() - start;
      
      log(`JSON Benchmark Results:`, 'success');
      log(`  C++ Time: ${cppDuration.toFixed(3)} ms`, 'info');
      log(`  JS Overhead: ${(jsOverhead - cppDuration).toFixed(3)} ms`, 'info');
      log(`  Total (JS->C++->JS): ${jsOverhead.toFixed(3)} ms`, 'info');
      log(`  Per iteration (C++): ${(cppDuration / 1000).toFixed(3)} ms (${(cppDuration).toFixed(1)} µs)`, 'info');

      // Cleanup
      log('\nDestroying TestObject...', 'info');
      testObj.destroy();
      log('TestObject destroyed', 'success');
      log('=== JSON BENCHMARK COMPLETE ===', 'success');

    } catch (error) {
      log(`JSON Benchmark failed: ${error}`, 'error');
    }
  }

  async function runJsonBenchmark2() {
    try {
      log('=== STARTING JSON ARRAY BENCHMARK ===', 'info');
      // Create TestObject
      log('Creating TestObject...', 'info');
      const testObj = await TestObject.create() as TestObject;
      log(`TestObject created: ${testObj.handle}`, 'success');

      log('Running JSON array deserialization benchmark (1000 iterations in C++)...', 'info');
      const start = performance.now();
      const cppDuration = await testObj.jsonBench2();
      const jsOverhead = performance.now() - start;

      log(`JSON Array Benchmark Results:`, 'success');
      log(`  C++ Time: ${cppDuration.toFixed(3)} ms`, 'info');
      log(`  JS Overhead: ${(jsOverhead - cppDuration).toFixed(3)} ms`, 'info');
      log(`  Total (JS->C++->JS): ${jsOverhead.toFixed(3)} ms`, 'info');
      log(`  Per iteration (C++): ${(cppDuration / 1000).toFixed(3)} ms (${(cppDuration).toFixed(1)} µs)`, 'info');

      // Cleanup
      log('\nDestroying TestObject...', 'info');
      testObj.destroy();
      log('TestObject destroyed', 'success');
      log('=== JSON ARRAY BENCHMARK COMPLETE ===', 'success');
    } catch (error) {
      log(`JSON Array Benchmark failed: ${error}`, 'error');
    }
  }

  async function runBenchmark() {
    try {
      log('=== STARTING BENCHMARK ===', 'info');
      
      // Create TestObject
      log('Creating TestObject...', 'info');
      const testObj = await TestObject.create() as TestObject;
      log(`TestObject created: ${testObj.handle}`, 'success');

      const iterations = 1000;
      const warmupRuns = 50;
      testObj.asyncProcess('f.a.b.');

      // ===== SYNC BENCHMARK =====
      log(`\n--- Sync Benchmark (${iterations} iterations) ---`, 'info');
      
      // Warmup
      for (let i = 0; i < warmupRuns; i++) {
        await testObj.benchmarkSync(i);
      }

      const syncTimes: number[] = [];
      for (let i = 0; i < iterations; i++) {
        const start = performance.now();
        await testObj.benchmarkSync(i);
        const elapsed = performance.now() - start;
        syncTimes.push(elapsed);
      }

      const syncStats = calculateStats(syncTimes);
      log(`Sync Results (${iterations}x):`, 'success');
      log(`  Total: ${syncStats.total.toFixed(2)} ms`, 'info');
      log(`  Avg:   ${syncStats.avg.toFixed(3)} ms (${(syncStats.avg * 1000).toFixed(1)} µs)`, 'info');
      log(`  Min:   ${syncStats.min.toFixed(3)} ms (${(syncStats.min * 1000).toFixed(1)} µs)`, 'info');
      log(`  Max:   ${syncStats.max.toFixed(3)} ms (${(syncStats.max * 1000).toFixed(1)} µs)`, 'info');
      log(`  StdDev: ${syncStats.stdev.toFixed(3)} ms (${(syncStats.stdev * 1000).toFixed(1)} µs)`, 'info');

      // ===== ASYNC BENCHMARK =====
      log(`\n--- Async Benchmark (${iterations} iterations) ---`, 'info');
      
      // Warmup
      for (let i = 0; i < warmupRuns; i++) {
        await testObj.benchmarkAsync(i);
      }

      const asyncTimes: number[] = [];
      for (let i = 0; i < iterations; i++) {
        const start = performance.now();
        await testObj.benchmarkAsync(i);
        const elapsed = performance.now() - start;
        asyncTimes.push(elapsed);
      }

      const asyncStats = calculateStats(asyncTimes);
      log(`Async Results (${iterations}x):`, 'success');
      log(`  Total: ${asyncStats.total.toFixed(2)} ms`, 'info');
      log(`  Avg:   ${asyncStats.avg.toFixed(3)} ms (${(asyncStats.avg * 1000).toFixed(1)} µs)`, 'info');
      log(`  Min:   ${asyncStats.min.toFixed(3)} ms (${(asyncStats.min * 1000).toFixed(1)} µs)`, 'info');
      log(`  Max:   ${asyncStats.max.toFixed(3)} ms (${(asyncStats.max * 1000).toFixed(1)} µs)`, 'info');
      log(`  StdDev: ${asyncStats.stdev.toFixed(3)} ms (${(asyncStats.stdev * 1000).toFixed(1)} µs)`, 'info');

      // Comparison
      log(`\n--- Comparison ---`, 'info');
      log(`  Async overhead: ${(asyncStats.avg - syncStats.avg).toFixed(3)} ms (${((asyncStats.avg - syncStats.avg) * 1000).toFixed(1)} µs)`, 'info');
      log(`  Async is ${(asyncStats.avg / syncStats.avg).toFixed(2)}x slower than Sync`, 'info');

      // Cleanup
      log('\nDestroying TestObject...', 'info');
      testObj.destroy();
      log('TestObject destroyed', 'success');
      log('=== BENCHMARK COMPLETE ===', 'success');

    } catch (error) {
      log(`Benchmark failed: ${error}`, 'error');
    }
  }

  onMount(() => {
    log('WebBridge Demo initialized 🚀', 'success');
  });
</script>

<main>
  <div class="container">
    <h1>🌉 WebBridge Svelte Demo</h1>

    <div class="status-bar">
      <div class="status-indicator" class:active={obj !== null}></div>
      <span>{obj ? 'Object active' : 'No object'}</span>
    </div>

    <!-- Control Buttons -->
    <div class="controls">
      <button on:click={createObject} disabled={obj !== null}>
        Create Object
      </button>
      <button on:click={destroyObject} disabled={obj === null}>
        Destroy Object
      </button>
      <button on:click={callBar} disabled={obj === null}>
        Call bar()
      </button>
      <button on:click={callFoo} disabled={obj === null}>
        Call foo()
      </button>
      <button on:click={callFile} disabled={obj === null}>
        Call file()
      </button>
      <button on:click={callTestVectors} disabled={obj === null}>
        Test Vectors
      </button>
      <button on:click={callThrowError} disabled={obj === null} class="error-btn">
        Throw Error 💥
      </button>
      <button on:click={callMultiParamTest} disabled={obj === null}>
        Test Multi-Params 🎯
      </button>
      <button on:click={clearLog}>
        Clear Log
      </button>
      <button on:click={runBenchmark} class="benchmark-btn">
        🚀 Run Benchmark (TestObject)
      </button>
      <button on:click={runJsonBenchmark} class="benchmark-btn">
        📊 Run JSON Benchmark
      </button>
      <button on:click={runJsonBenchmark2} class="benchmark-btn">
        📊 Run JSON Array Benchmark
      </button>
      <button on:click={testTestObject8} class="test-btn">
        🧪 Test TestObject8
      </button>
    </div>

    <!-- Property Display - Reactive via WebBridge Stores! -->
    <div class="properties">
      <h2>Properties (auto-synced via WebBridge)</h2>
      <div class="property-grid">
        {#if obj && aBool && strProp && counter && numbers && status && pod}
          <div class="property">
            <span class="property-label">aBool:</span>
            <span class="value">{$aBool}</span>
          </div>
          <div class="property">
            <span class="property-label">strProp:</span>
            <span class="value">{$strProp || '(empty)'}</span>
          </div>
          <div class="property">
            <span class="property-label">counter:</span>
            <span class="value">{$counter}</span>
          </div>
          <div class="property">
            <span class="property-label">numbers:</span>
            <span class="value">[{$numbers ? $numbers.join(', ') : ''}]</span>
          </div>
          <div class="property">
            <span class="property-label">status:</span>
            <span class="value status-{($status as any)?.toLowerCase?.() || 'idle'}">{$status as any || 'Idle'}</span>
          </div>
          {#if $pod}
          <div class="property">
            <span class="property-label">pod:</span>
            <span class="value">{`{ a: ${($pod as any).a}, b: ${($pod as any).b} }`}</span>
          </div>
          {/if}
        {:else}
          <p>No object created yet.</p>
        {/if}
      </div>
    </div>

    <!-- Image Display -->
    <div class="image-section">
      <h2>🖼️ Embedded Image</h2>
      <img src={bildUrl} alt="Beispielbild" class="demo-image" />
      <p class="info-text">This image is bundled with the app and served from embedded resources.</p>
    </div>

    <!-- Log Section -->
    <div class="log-section">
      <h2>Console Log</h2>
      <div class="log-output">
        {#each logs as logEntry}
          <div class="log-entry">{logEntry}</div>
        {/each}
      </div>
    </div>
  </div>
</main>

<style>
  :global(body) {
    margin: 0;
    padding: 0;
    font-family: -apple-system, BlinkMacSystemFont, 'Segoe UI', Roboto, 'Helvetica Neue', Arial, sans-serif;
    background: linear-gradient(135deg, #667eea 0%, #764ba2 100%);
  }

  main {
    min-height: 100vh;
    padding: 20px;
  }

  .container {
    max-width: 900px;
    margin: 0 auto;
    background: white;
    border-radius: 12px;
    padding: 30px;
    box-shadow: 0 10px 40px rgba(0, 0, 0, 0.2);
  }

  h1 {
    text-align: center;
    color: #333;
    margin-bottom: 20px;
  }

  .status-bar {
    display: flex;
    align-items: center;
    gap: 10px;
    padding: 10px 15px;
    background: #f5f5f5;
    border-radius: 8px;
    margin-bottom: 20px;
  }

  .status-indicator {
    width: 12px;
    height: 12px;
    border-radius: 50%;
    background: #ccc;
    transition: background 0.3s;
  }

  .status-indicator.active {
    background: #00ff00;
    box-shadow: 0 0 10px #00ff00;
  }

  .controls {
    display: flex;
    flex-wrap: wrap;
    gap: 10px;
    margin-bottom: 20px;
  }

  button {
    padding: 10px 20px;
    background: #667eea;
    color: white;
    border: none;
    border-radius: 6px;
    cursor: pointer;
    font-size: 14px;
    transition: all 0.3s;
  }

  button:hover:not(:disabled) {
    background: #5568d3;
    transform: translateY(-2px);
    box-shadow: 0 4px 12px rgba(102, 126, 234, 0.4);
  }

  button:disabled {
    background: #ccc;
    cursor: not-allowed;
    transform: none;
  }

  .error-btn {
    background: #e91e63;
  }

  .error-btn:hover:not(:disabled) {
    background: #c2185b;
  }

  .benchmark-btn {
    background: #ff9800;
    font-weight: bold;
  }

  .benchmark-btn:hover {
    background: #f57c00;
  }

  .test-btn {
    background: #00bcd4;
    font-weight: bold;
  }

  .test-btn:hover {
    background: #0097a7;
  }

  .properties {
    background: #f9f9f9;
    padding: 20px;
    border-radius: 8px;
    margin-bottom: 20px;
  }

  .properties h2 {
    margin-top: 0;
    color: #333;
    font-size: 18px;
  }

  .info-text {
    margin-top: 15px;
    padding: 10px;
    background: rgba(255, 255, 255, 0.7);
    border-radius: 6px;
    font-size: 13px;
    color: #555;
  }

  .property-grid {
    display: grid;
    gap: 12px;
  }

  .property {
    display: flex;
    justify-content: space-between;
    padding: 10px;
    background: white;
    border-radius: 6px;
    border-left: 3px solid #667eea;
  }

  .property .property-label {
    font-weight: 600;
    color: #555;
  }

  .property .value {
    color: #333;
    font-family: 'Courier New', monospace;
  }

  .value.status-idle { color: #888; }
  .value.status-running { color: #ff9800; }
  .value.status-completed { color: #4caf50; }
  .value.status-error { color: #f44336; }

  .image-section {
    margin-top: 20px;
  }

  .image-section h2 {
    margin-top: 0;
    color: #333;
    font-size: 18px;
  }

  .demo-image {
    max-width: 100%;
    height: auto;
    border-radius: 8px;
    box-shadow: 0 4px 6px rgba(0, 0, 0, 0.1);
    margin: 10px 0;
  }

  .log-section {
    margin-top: 20px;
  }

  .log-section h2 {
    margin-top: 0;
    color: #333;
    font-size: 18px;
  }

  .log-output {
    background: #1e1e1e;
    color: #d4d4d4;
    padding: 15px;
    border-radius: 6px;
    font-family: 'Courier New', monospace;
    font-size: 13px;
    max-height: 300px;
    overflow-y: auto;
  }

  .log-entry {
    margin-bottom: 5px;
    line-height: 1.5;
  }
</style>
