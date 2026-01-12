#pragma once

#include "TestObject_registration.h"
#include "TestObject1_registration.h"
#include "TestObject2_registration.h"
#include "TestObject3_registration.h"
#include "TestObject4_registration.h"
#include "TestObject5_registration.h"
#include "TestObject6_registration.h"
#include "TestObject7_registration.h"
#include "TestObject8_registration.h"
#include "TestObject9_registration.h"
#include "TestObject10_registration.h"
#include "TestObject11_registration.h"
#include "TestObject12_registration.h"
#include "TestObject13_registration.h"
#include "TestObject14_registration.h"
#include "TestObject15_registration.h"
#include "TestObject16_registration.h"
#include "TestObject17_registration.h"
#include "TestObject18_registration.h"
#include "TestObject19_registration.h"
#include "TestObject20_registration.h"
#include "TestObject21_registration.h"
#include "TestObject22_registration.h"
#include "TestObject23_registration.h"
#include "TestObject24_registration.h"
#include "TestObject25_registration.h"
#include "TestObject26_registration.h"
#include "TestObject27_registration.h"
#include "TestObject28_registration.h"
#include "TestObject29_registration.h"
#include "TestObject30_registration.h"
#include "TestObject31_registration.h"
#include "TestObject32_registration.h"
#include "TestObject33_registration.h"
#include "TestObject34_registration.h"
#include "TestObject35_registration.h"
#include "MyObject_registration.h"
#include "ResourceServer.h"
#include "webbridge/Object.h"
#include "webbridge/Error.h"
#include <webview/webview.h>
#include <portable-file-dialogs.h>
#include <iostream>
#include <thread>
#include <chrono>
#include <cstdio>
#include <fstream>
#include <iostream>

int WINAPI WinMain(HINSTANCE /*hInst*/, HINSTANCE /*hPrevInst*/,
	LPSTR /*lpCmdLine*/, int /*nCmdShow*/)
{
	//AllocConsole();
	freopen("CONOUT$", "w", stdout);
	freopen("CONOUT$", "w", stderr);
	
	// Open timing log file
	std::ofstream timing_log("timing_trace.log", std::ios::app);
	std::streambuf* cout_backup = std::cout.rdbuf();
	std::cout.rdbuf(timing_log.rdbuf());

	try {
		// Start HTTP server with embedded resources
		ResourceServer server;
		if (!server.start()) {
			std::cerr << "Failed to start resource server\n";
			return 1;
		}
		
		std::cout << "Resource server running on " << server.get_url() << "\n";
		
#ifdef _DEBUG
		webview::webview w(true, nullptr);
#else
		webview::webview w(false, nullptr);

		// Disable context menu in release mode
		w.eval(R"(
			document.addEventListener('contextmenu', (e) => {
				e.preventDefault();
				return false;
			});
		)");
#endif
		w.set_title("WebBridge Demo");
		w.set_size(900, 700, WEBVIEW_HINT_NONE);

		// Register error handler
		webbridge::set_error_handler([](webbridge::error& err, const std::exception& ex) {
			pfd::message(
				"Error " + std::to_string(err.code),
				err.message,
				pfd::choice::ok,
				pfd::icon::error
			);
		});

		
		// Register all 35 TestObjects with timing measurements
		std::cout << "\n========== REGISTERING 35 TEST OBJECTS ==========\n" << std::endl;
		auto total_start = std::chrono::high_resolution_clock::now();
		
		auto start1 = std::chrono::high_resolution_clock::now();
		webbridge::impl::register_TestObject1(&w);
		auto end1 = std::chrono::high_resolution_clock::now();
		std::cout << "TestObject1: " << std::chrono::duration_cast<std::chrono::milliseconds>(end1 - start1).count() << " ms" << std::endl;
		
		auto start2 = std::chrono::high_resolution_clock::now();
		webbridge::impl::register_TestObject2(&w);
		auto end2 = std::chrono::high_resolution_clock::now();
		std::cout << "TestObject2: " << std::chrono::duration_cast<std::chrono::milliseconds>(end2 - start2).count() << " ms" << std::endl;
		
		auto start3 = std::chrono::high_resolution_clock::now();
		webbridge::impl::register_TestObject3(&w);
		auto end3 = std::chrono::high_resolution_clock::now();
		std::cout << "TestObject3: " << std::chrono::duration_cast<std::chrono::milliseconds>(end3 - start3).count() << " ms" << std::endl;
		
		auto start4 = std::chrono::high_resolution_clock::now();
		webbridge::impl::register_TestObject4(&w);
		auto end4 = std::chrono::high_resolution_clock::now();
		std::cout << "TestObject4: " << std::chrono::duration_cast<std::chrono::milliseconds>(end4 - start4).count() << " ms" << std::endl;
		
		auto start5 = std::chrono::high_resolution_clock::now();
		webbridge::impl::register_TestObject5(&w);
		auto end5 = std::chrono::high_resolution_clock::now();
		std::cout << "TestObject5: " << std::chrono::duration_cast<std::chrono::milliseconds>(end5 - start5).count() << " ms" << std::endl;
		
		auto start6 = std::chrono::high_resolution_clock::now();
		webbridge::impl::register_TestObject6(&w);
		auto end6 = std::chrono::high_resolution_clock::now();
		std::cout << "TestObject6: " << std::chrono::duration_cast<std::chrono::milliseconds>(end6 - start6).count() << " ms" << std::endl;
		
		auto start7 = std::chrono::high_resolution_clock::now();
		webbridge::impl::register_TestObject7(&w);
		auto end7 = std::chrono::high_resolution_clock::now();
		std::cout << "TestObject7: " << std::chrono::duration_cast<std::chrono::milliseconds>(end7 - start7).count() << " ms" << std::endl;
		
		auto start8 = std::chrono::high_resolution_clock::now();
		webbridge::impl::register_TestObject8(&w);
		auto end8 = std::chrono::high_resolution_clock::now();
		std::cout << "TestObject8: " << std::chrono::duration_cast<std::chrono::milliseconds>(end8 - start8).count() << " ms" << std::endl;
		
		auto start9 = std::chrono::high_resolution_clock::now();
		webbridge::impl::register_TestObject9(&w);
		auto end9 = std::chrono::high_resolution_clock::now();
		std::cout << "TestObject9: " << std::chrono::duration_cast<std::chrono::milliseconds>(end9 - start9).count() << " ms" << std::endl;
		
		auto start10 = std::chrono::high_resolution_clock::now();
		webbridge::impl::register_TestObject10(&w);
		auto end10 = std::chrono::high_resolution_clock::now();
		std::cout << "TestObject10: " << std::chrono::duration_cast<std::chrono::milliseconds>(end10 - start10).count() << " ms" << std::endl;
		
		auto start11 = std::chrono::high_resolution_clock::now();
		webbridge::impl::register_TestObject11(&w);
		auto end11 = std::chrono::high_resolution_clock::now();
		std::cout << "TestObject11: " << std::chrono::duration_cast<std::chrono::milliseconds>(end11 - start11).count() << " ms" << std::endl;
		
		auto start12 = std::chrono::high_resolution_clock::now();
		webbridge::impl::register_TestObject12(&w);
		auto end12 = std::chrono::high_resolution_clock::now();
		std::cout << "TestObject12: " << std::chrono::duration_cast<std::chrono::milliseconds>(end12 - start12).count() << " ms" << std::endl;
		
		auto start13 = std::chrono::high_resolution_clock::now();
		webbridge::impl::register_TestObject13(&w);
		auto end13 = std::chrono::high_resolution_clock::now();
		std::cout << "TestObject13: " << std::chrono::duration_cast<std::chrono::milliseconds>(end13 - start13).count() << " ms" << std::endl;
		
		auto start14 = std::chrono::high_resolution_clock::now();
		webbridge::impl::register_TestObject14(&w);
		auto end14 = std::chrono::high_resolution_clock::now();
		std::cout << "TestObject14: " << std::chrono::duration_cast<std::chrono::milliseconds>(end14 - start14).count() << " ms" << std::endl;
		
		auto start15 = std::chrono::high_resolution_clock::now();
		webbridge::impl::register_TestObject15(&w);
		auto end15 = std::chrono::high_resolution_clock::now();
		std::cout << "TestObject15: " << std::chrono::duration_cast<std::chrono::milliseconds>(end15 - start15).count() << " ms" << std::endl;
		
		auto start16 = std::chrono::high_resolution_clock::now();
		webbridge::impl::register_TestObject16(&w);
		auto end16 = std::chrono::high_resolution_clock::now();
		std::cout << "TestObject16: " << std::chrono::duration_cast<std::chrono::milliseconds>(end16 - start16).count() << " ms" << std::endl;
		
		auto start17 = std::chrono::high_resolution_clock::now();
		webbridge::impl::register_TestObject17(&w);
		auto end17 = std::chrono::high_resolution_clock::now();
		std::cout << "TestObject17: " << std::chrono::duration_cast<std::chrono::milliseconds>(end17 - start17).count() << " ms" << std::endl;
		
		auto start18 = std::chrono::high_resolution_clock::now();
		webbridge::impl::register_TestObject18(&w);
		auto end18 = std::chrono::high_resolution_clock::now();
		std::cout << "TestObject18: " << std::chrono::duration_cast<std::chrono::milliseconds>(end18 - start18).count() << " ms" << std::endl;
		
		auto start19 = std::chrono::high_resolution_clock::now();
		webbridge::impl::register_TestObject19(&w);
		auto end19 = std::chrono::high_resolution_clock::now();
		std::cout << "TestObject19: " << std::chrono::duration_cast<std::chrono::milliseconds>(end19 - start19).count() << " ms" << std::endl;
		
		auto start20 = std::chrono::high_resolution_clock::now();
		webbridge::impl::register_TestObject20(&w);
		auto end20 = std::chrono::high_resolution_clock::now();
		std::cout << "TestObject20: " << std::chrono::duration_cast<std::chrono::milliseconds>(end20 - start20).count() << " ms" << std::endl;
		
		auto start21 = std::chrono::high_resolution_clock::now();
		webbridge::impl::register_TestObject21(&w);
		auto end21 = std::chrono::high_resolution_clock::now();
		std::cout << "TestObject21: " << std::chrono::duration_cast<std::chrono::milliseconds>(end21 - start21).count() << " ms" << std::endl;
		
		auto start22 = std::chrono::high_resolution_clock::now();
		webbridge::impl::register_TestObject22(&w);
		auto end22 = std::chrono::high_resolution_clock::now();
		std::cout << "TestObject22: " << std::chrono::duration_cast<std::chrono::milliseconds>(end22 - start22).count() << " ms" << std::endl;
		
		auto start23 = std::chrono::high_resolution_clock::now();
		webbridge::impl::register_TestObject23(&w);
		auto end23 = std::chrono::high_resolution_clock::now();
		std::cout << "TestObject23: " << std::chrono::duration_cast<std::chrono::milliseconds>(end23 - start23).count() << " ms" << std::endl;
		
		auto start24 = std::chrono::high_resolution_clock::now();
		webbridge::impl::register_TestObject24(&w);
		auto end24 = std::chrono::high_resolution_clock::now();
		std::cout << "TestObject24: " << std::chrono::duration_cast<std::chrono::milliseconds>(end24 - start24).count() << " ms" << std::endl;
		
		auto start25 = std::chrono::high_resolution_clock::now();
		webbridge::impl::register_TestObject25(&w);
		auto end25 = std::chrono::high_resolution_clock::now();
		std::cout << "TestObject25: " << std::chrono::duration_cast<std::chrono::milliseconds>(end25 - start25).count() << " ms" << std::endl;
		
		auto start26 = std::chrono::high_resolution_clock::now();
		webbridge::impl::register_TestObject26(&w);
		auto end26 = std::chrono::high_resolution_clock::now();
		std::cout << "TestObject26: " << std::chrono::duration_cast<std::chrono::milliseconds>(end26 - start26).count() << " ms" << std::endl;
		
		auto start27 = std::chrono::high_resolution_clock::now();
		webbridge::impl::register_TestObject27(&w);
		auto end27 = std::chrono::high_resolution_clock::now();
		std::cout << "TestObject27: " << std::chrono::duration_cast<std::chrono::milliseconds>(end27 - start27).count() << " ms" << std::endl;
		
		auto start28 = std::chrono::high_resolution_clock::now();
		webbridge::impl::register_TestObject28(&w);
		auto end28 = std::chrono::high_resolution_clock::now();
		std::cout << "TestObject28: " << std::chrono::duration_cast<std::chrono::milliseconds>(end28 - start28).count() << " ms" << std::endl;
		
		auto start29 = std::chrono::high_resolution_clock::now();
		webbridge::impl::register_TestObject29(&w);
		auto end29 = std::chrono::high_resolution_clock::now();
		std::cout << "TestObject29: " << std::chrono::duration_cast<std::chrono::milliseconds>(end29 - start29).count() << " ms" << std::endl;
		
		auto start30 = std::chrono::high_resolution_clock::now();
		webbridge::impl::register_TestObject30(&w);
		auto end30 = std::chrono::high_resolution_clock::now();
		std::cout << "TestObject30: " << std::chrono::duration_cast<std::chrono::milliseconds>(end30 - start30).count() << " ms" << std::endl;
		
		auto start31 = std::chrono::high_resolution_clock::now();
		webbridge::impl::register_TestObject31(&w);
		auto end31 = std::chrono::high_resolution_clock::now();
		std::cout << "TestObject31: " << std::chrono::duration_cast<std::chrono::milliseconds>(end31 - start31).count() << " ms" << std::endl;
		
		auto start32 = std::chrono::high_resolution_clock::now();
		webbridge::impl::register_TestObject32(&w);
		auto end32 = std::chrono::high_resolution_clock::now();
		std::cout << "TestObject32: " << std::chrono::duration_cast<std::chrono::milliseconds>(end32 - start32).count() << " ms" << std::endl;
		
		auto start33 = std::chrono::high_resolution_clock::now();
		webbridge::impl::register_TestObject33(&w);
		auto end33 = std::chrono::high_resolution_clock::now();
		std::cout << "TestObject33: " << std::chrono::duration_cast<std::chrono::milliseconds>(end33 - start33).count() << " ms" << std::endl;
		
		auto start34 = std::chrono::high_resolution_clock::now();
		webbridge::impl::register_TestObject34(&w);
		auto end34 = std::chrono::high_resolution_clock::now();
		std::cout << "TestObject34: " << std::chrono::duration_cast<std::chrono::milliseconds>(end34 - start34).count() << " ms" << std::endl;
		
		auto start35 = std::chrono::high_resolution_clock::now();
		webbridge::impl::register_TestObject35(&w);
		auto end35 = std::chrono::high_resolution_clock::now();
		std::cout << "TestObject35: " << std::chrono::duration_cast<std::chrono::milliseconds>(end35 - start35).count() << " ms" << std::endl;
		
		auto total_end = std::chrono::high_resolution_clock::now();
		auto total_duration = std::chrono::duration_cast<std::chrono::milliseconds>(total_end - total_start);
		std::cout << "\n========== TOTAL TIME: " << total_duration.count() << " ms ==========\n" << std::endl;
		
		// Register TestObject BEFORE MyObject
		std::cout << "Starting register_type for TestObject..." << std::endl;
		auto start_test = std::chrono::high_resolution_clock::now();
		webbridge::impl::register_TestObject(&w);
		auto end_test = std::chrono::high_resolution_clock::now();
		auto duration_test = std::chrono::duration_cast<std::chrono::milliseconds>(end_test - start_test);
		std::cout << "register_type for TestObject took " << duration_test.count() << " ms" << std::endl;
		
		// Register type -> needs to be created in js
		std::cout << "Starting register_type for MyObject..." << std::endl;
		auto start = std::chrono::high_resolution_clock::now();
		webbridge::impl::register_MyObject(&w);
		auto end = std::chrono::high_resolution_clock::now();
		auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
		std::cout << "register_type took " << duration.count() << " ms" << std::endl;
		
		// Navigate FIRST so the frontend (with WebbridgeRuntime) is loaded
		w.navigate(server.get_url());
		w.run();
		
		// Restore cout and close timing log
		std::cout.rdbuf(cout_backup);
		timing_log.close();
	} catch (const webview::exception &e) {
		std::cerr << e.what() << '\n';
		return 1;
	}

	return 0;
}
