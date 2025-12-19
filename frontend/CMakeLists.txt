find_program(NPM_EXECUTABLE npm REQUIRED)
message(STATUS "Found npm: ${NPM_EXECUTABLE}")

add_custom_target(webbridge_gen
	COMMENT "Generating TypeScript types from C++ headers"
)

webbridge_generate(
	TARGET webbridge_gen
	FILES
		${CMAKE_SOURCE_DIR}/src/MyObject.h
	LANGUAGE ts
	OUTPUT_DIR ${CMAKE_CURRENT_SOURCE_DIR}/src
)

set_target_properties(webbridge_gen
	PROPERTIES FOLDER "frontend"
)

set(NPM_INSTALL_STAMP ${CMAKE_CURRENT_BINARY_DIR}/npm_install.stamp)

add_custom_command(
	OUTPUT ${NPM_INSTALL_STAMP}
	COMMAND ${NPM_EXECUTABLE} install
	COMMAND ${CMAKE_COMMAND} -E touch ${NPM_INSTALL_STAMP}
	WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
	DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/package.json
	COMMENT "Installing npm dependencies"
)

add_custom_target(npm_install
	DEPENDS ${NPM_INSTALL_STAMP}
)

set_target_properties(npm_install
	PROPERTIES FOLDER "frontend"
)

set(ASSETS
	bild.jpg
)

set(FRONTEND_SRC_FILES
	src/app.css
	src/App.svelte
	src/main.ts
)

foreach(asset IN LISTS ASSETS)
	list(APPEND FRONTEND_SRC_FILES "src/assets/${asset}")
endforeach()

add_custom_command(
	OUTPUT ${CMAKE_CURRENT_SOURCE_DIR}/dist/index.html
	COMMAND ${NPM_EXECUTABLE} run build
	WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
	DEPENDS
		vite.config.ts
		svelte.config.js
		tsconfig.json
		index.html
		${FRONTEND_SRC_FILES}
	COMMENT "Building Svelte frontend"
)

add_custom_target(frontend_build ALL
	DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/dist/index.html
)

add_dependencies(frontend_build npm_install webbridge_gen)

set_target_properties(frontend_build
	PROPERTIES FOLDER "frontend"
)

set(FRONTEND_DIST_FILES
	dist/assets/index.css
	dist/assets/index.js
	dist/index.html
)

foreach(asset IN LISTS ASSETS)
	list(APPEND FRONTEND_DIST_FILES "dist/assets/${asset}")
endforeach()

cmrc_add_resource_library(
	frontend_resources
	NAMESPACE frontend
	WHENCE ${CMAKE_CURRENT_SOURCE_DIR}/dist
	${FRONTEND_DIST_FILES}
)

add_dependencies(frontend_resources frontend_build)

set_target_properties(frontend_resources
	PROPERTIES FOLDER "frontend"
)
