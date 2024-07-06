#include <vk_engine.h>

#undef main

int main(int argc, char* argv[])
{
	VulkanEngine engine;

	system("F:\\MelloEngine\\shaders\\compile_shaders.bat");
	engine.init();	
	
	engine.run();	

	engine.cleanup();	

	return 0;
}
