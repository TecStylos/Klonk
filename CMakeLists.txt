cmake_minimum_required(VERSION 3.8)

project("klonk")

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED TRUE)

add_executable(
	klonk
	"src/main.cpp"

	"src/stb/stb_image.cpp"
	"src/stb/stb_image_write.cpp"

	"src/ui/Image.cpp"
	"src/ui/framebuffer.cpp"
	"src/ui/UserInterface.cpp"
	"src/ui/GenTextImage.cpp"
	"src/ui/MakeHomeButton.cpp"
	
	"src/apps/AppHome.cpp"
	"src/apps/AppClock.cpp"
	"src/apps/AppSpotify.cpp"
	
	"src/spotify/spotify.cpp"
	"src/spotify/Response.cpp"
)

target_include_directories(
	klonk PUBLIC
	"src"
)

set(THREADS_PREFER_PTHREAD_FLAG ON)
find_package(Threads REQUIRED)
target_link_libraries(klonk PRIVATE Threads::Threads)
