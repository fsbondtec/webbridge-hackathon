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

		// Register type -> needs to be created in js
	std::cout << "Starting register_type for MyObject..." << std::endl;
	auto start = std::chrono::high_resolution_clock::now();
	webbridge::register_type<MyObject>(&w);
	auto end = std::chrono::high_resolution_clock::now();
	auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
	std::cout << "register_type took " << duration.count() << " ms" << std::endl;
		// Publish a existing object to js
		auto globalObject = std::make_shared<MyObject>("Published from C++!");
		globalObject->strProp = "Published from C++!";
		globalObject->counter = 42;
		globalObject->aBool = true;
		webbridge::publish_object<MyObject>(&w, "globalMyObject", globalObject);
		std::thread([globalObject]() {
			std::this_thread::sleep_for(std::chrono::seconds(2));
			globalObject->strProp = "Updated from C++ after 2s";
			globalObject->counter = 100;
			
			std::this_thread::sleep_for(std::chrono::seconds(2));
			globalObject->aEvent.emit(999, true);
		}).detach();

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
