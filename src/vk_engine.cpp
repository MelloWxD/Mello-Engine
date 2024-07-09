
#include <vk_engine.h>

#include <glm/gtx/transform.hpp>



#define VMA_IMPLEMENTATION
#include "vk_mem_alloc.h"
#include <glm/gtx/transform.hpp>
#include <glm/gtx/quaternion.hpp>

float Deg2Rad(float angDegrees)
{
	return angDegrees * (glm::pi<float>() / 180);
}
float Rad2Deg(float angRad)
{
	return(angRad * (180 / glm::pi<float>()));
}



bool is_visible(const RenderObject& obj, const m4& viewproj) 
{

	std::array<v3, 8> corners
	{
		v3 { 1, 1, 1 },
		v3 { 1, 1, -1 },
		v3 { 1, -1, 1 },
		v3 { 1, -1, -1 },
		v3 { -1, 1, 1 },
		v3 { -1, 1, -1 },
		v3 { -1, -1, 1 },
		v3 { -1, -1, -1 },
	};

	m4 matrix = viewproj * obj.transform;

	v3 min = { 1.5, 1.5, 1.5 };
	v3 max = { -1.5, -1.5, -1.5 };

	for (int c = 0; c < 8; c++) {
		// project each corner into clip space
		v4 v = matrix * v4(obj.bounds.origin + (corners[c] * obj.bounds.extents), 1.f);

		// perspective correction
		v.x = v.x / v.w;
		v.y = v.y / v.w;
		v.z = v.z / v.w;

		min = glm::min(glm::vec3{ v.x, v.y, v.z }, min);
		max = glm::max(glm::vec3{ v.x, v.y, v.z }, max);
	}

	// check the clip space box is within the view
	if (min.z > 1.f || max.z < 0.f || min.x > 1.f || max.x < -1.f || min.y > 1.f || max.y < -1.f) {
		return false;
	}
	else {
		return true;
	}
}
void VulkanEngine::init()
{
	// We initialize SDL and create a window with it. 
	SDL_Init(SDL_INIT_VIDEO);

	SDL_WindowFlags window_flags = (SDL_WindowFlags)(SDL_WINDOW_VULKAN | SDL_WINDOW_RESIZABLE);

	
	_window = SDL_CreateWindow(
		"Mello Engine",
		SDL_WINDOWPOS_UNDEFINED,
		SDL_WINDOWPOS_UNDEFINED,
		_windowExtent.width,
		_windowExtent.height,
		window_flags
	);
	
	init_vulkan();

	init_swapchain();

	init_commands();

	init_sync_structures();

	init_descriptors();

	init_pipelines();


	init_default_data();

	init_renderables();

	init_imgui();

	mainCamera.Velocity = v3(0.f);
	mainCamera.Position = v3(0);

	mainCamera.pitch = 0;
	mainCamera.yaw = 0;

	//everything went fine
	_isInitialized = true;
}
void VulkanEngine::cleanup()
{	
	if (_isInitialized) 
	{

		vkDeviceWaitIdle(_device);
		loadedGLTFs.clear();

		for (auto& frame : _frames) 
		{
			frame._deletionQueue.flush();
		}
		for (int i = 0; i < FRAME_OVERLAP; i++) {

			_frames[i]._deletionQueue.flush();

			vkDestroyCommandPool(_device, _frames[i]._commandPool, nullptr);
			//destroy sync objects
			vkDestroyFence(_device, _frames[i]._renderFence, nullptr);
			vkDestroySemaphore(_device, _frames[i]._renderSemaphore, nullptr);
			vkDestroySemaphore(_device, _frames[i]._swapchainSemaphore, nullptr);
		}
		_mainDeletionQueue.flush();

		destroy_swapchain();

		vkDestroySurfaceKHR(_instance, _surface, nullptr);

		vmaDestroyAllocator(_allocator);

		vkDestroyDevice(_device, nullptr);
		vkb::destroy_debug_utils_messenger(_instance, _debug_messenger);
		vkDestroyInstance(_instance, nullptr);

		SDL_DestroyWindow(_window);
	}
}

void VulkanEngine::rebuild_swapchain()
{
	vkQueueWaitIdle(_graphicsQueue);

	vkb::SwapchainBuilder swapchainBuilder{ _chosenGPU,_device,_surface };

	SDL_GetWindowSizeInPixels(_window, (int*)&_windowExtent.width, (int*)&_windowExtent.height);

	vkDestroySwapchainKHR(_device, _swapchain, nullptr);
	vkDestroyImageView(_device, _drawImage.imageView, nullptr);
	vmaDestroyImage(_allocator, _drawImage.image, _drawImage.allocation);

	vkb::Swapchain vkbSwapchain = swapchainBuilder
		.use_default_format_selection()
		//use vsync present mode
		.set_desired_present_mode(VK_PRESENT_MODE_FIFO_KHR)
		.set_desired_extent(_windowExtent.width, _windowExtent.height)
		.add_image_usage_flags(VK_IMAGE_USAGE_TRANSFER_DST_BIT)
		.build()
		.value();

	//store swapchain and its related images
	_swapchain = vkbSwapchain.swapchain;
	m_swapchainImages = vkbSwapchain.get_images().value();
	m_swapchainImageViews = vkbSwapchain.get_image_views().value();

	_swapchainImageFormat = vkbSwapchain.image_format;

	//depth image size will match the window
	VkExtent3D drawImageExtent = {
		_windowExtent.width,
		_windowExtent.height,
		1
	};

	//hardcoding the depth format to 32 bit float
	_drawImage.imageFormat = VK_FORMAT_R16G16B16A16_SFLOAT;

	VkImageUsageFlags drawImageUsages{};
	drawImageUsages |= VK_IMAGE_USAGE_TRANSFER_SRC_BIT;
	drawImageUsages |= VK_IMAGE_USAGE_TRANSFER_DST_BIT;
	drawImageUsages |= VK_IMAGE_USAGE_STORAGE_BIT;

	VkImageCreateInfo rimg_info = vkinit::image_create_info(_drawImage.imageFormat, drawImageUsages, drawImageExtent);

	//for the draw image, we want to allocate it from gpu local memory
	VmaAllocationCreateInfo rimg_allocinfo = {};
	rimg_allocinfo.usage = VMA_MEMORY_USAGE_GPU_ONLY;
	rimg_allocinfo.requiredFlags = VkMemoryPropertyFlags(VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

	//allocate and create the image
	vmaCreateImage(_allocator, &rimg_info, &rimg_allocinfo, &_drawImage.image, &_drawImage.allocation, nullptr);

	//build a image-view for the draw image to use for rendering
	VkImageViewCreateInfo rview_info = vkinit::imageview_create_info(_drawImage.imageFormat, _drawImage.image, VK_IMAGE_ASPECT_COLOR_BIT);

	VK_CHECK(vkCreateImageView(_device, &rview_info, nullptr, &_drawImage.imageView));

	VkDescriptorImageInfo imgInfo{};
	imgInfo.imageLayout = VK_IMAGE_LAYOUT_GENERAL;
	imgInfo.imageView = _drawImage.imageView;

	VkWriteDescriptorSet cameraWrite = vkinit::write_descriptor_image(VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, _drawImageDescriptors, &imgInfo, 0);

	vkUpdateDescriptorSets(_device, 1, &cameraWrite, 0, nullptr);

	//add to deletion queues
	_mainDeletionQueue.push_function([&]() {
		vkDestroyImageView(_device, _drawImage.imageView, nullptr);
	vmaDestroyImage(_allocator, _drawImage.image, _drawImage.allocation);
	});
}


void VulkanEngine::draw()
{
	//wait until the gpu has finished rendering the last frame. Timeout of 1 second
	VK_CHECK(vkWaitForFences(_device, 1, &get_current_frame()._renderFence, true, 1000000000));

	get_current_frame()._deletionQueue.flush();
	get_current_frame()._frameDescriptors.clear_pools(_device);
	//request image from the swapchain
	uint32_t swapchainImageIndex;

	VkResult e = vkAcquireNextImageKHR(_device, _swapchain, 1000000000, get_current_frame()._swapchainSemaphore, nullptr, &swapchainImageIndex);
	if (e == VK_ERROR_OUT_OF_DATE_KHR) {
		resize_requested = true;
		return;
	}

	_drawExtent.height = std::min(_swapchainExtent.height, _drawImage.imageExtent.height) * renderScale;
	_drawExtent.width = std::min(_swapchainExtent.width, _drawImage.imageExtent.width) * renderScale;

	VK_CHECK(vkResetFences(_device, 1, &get_current_frame()._renderFence));

	//now that we are sure that the commands finished executing, we can safely reset the command buffer to begin recording again.
	VK_CHECK(vkResetCommandBuffer(get_current_frame()._mainCommandBuffer, 0));

	//naming it cmd for shorter writing
	VkCommandBuffer cmd = get_current_frame()._mainCommandBuffer;

	//begin the command buffer recording. We will use this command buffer exactly once, so we want to let vulkan know that
	VkCommandBufferBeginInfo cmdBeginInfo = vkinit::command_buffer_begin_info(VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT);

	//> draw_first
	VK_CHECK(vkBeginCommandBuffer(cmd, &cmdBeginInfo));

	// transition our main draw image into general layout so we can write into it
	// we will overwrite it all so we dont care about what was the older layout
	vkutil::transition_image(cmd, _drawImage.image, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_GENERAL);
	vkutil::transition_image(cmd, _depthImage.image, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL);

	draw_main(cmd);

	//transtion the draw image and the swapchain image into their correct transfer layouts
	vkutil::transition_image(cmd, _drawImage.image, VK_IMAGE_LAYOUT_GENERAL, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL);
	vkutil::transition_image(cmd, m_swapchainImages[swapchainImageIndex], VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);

	VkExtent2D extent;
	extent.height = _windowExtent.height;
	extent.width = _windowExtent.width;
	//< draw_first
	//> imgui_draw
	// execute a copy from the draw image into the swapchain
	vkutil::copy_image_to_image(cmd, _drawImage.image, m_swapchainImages[swapchainImageIndex], _drawExtent, _swapchainExtent);

	// set swapchain image layout to Attachment Optimal so we can draw it
	vkutil::transition_image(cmd, m_swapchainImages[swapchainImageIndex], VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);

	//draw imgui into the swapchain image
	draw_imgui(cmd, m_swapchainImageViews[swapchainImageIndex]);

	// set swapchain image layout to Present so we can draw it
	vkutil::transition_image(cmd, m_swapchainImages[swapchainImageIndex], VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, VK_IMAGE_LAYOUT_PRESENT_SRC_KHR);

	//finalize the command buffer (we can no longer add commands, but it can now be executed)
	VK_CHECK(vkEndCommandBuffer(cmd));

	//prepare the submission to the queue. 
	//we want to wait on the _presentSemaphore, as that semaphore is signaled when the swapchain is ready
	//we will signal the _renderSemaphore, to signal that rendering has finished

	VkCommandBufferSubmitInfo cmdinfo = vkinit::command_buffer_submit_info(cmd);

	VkSemaphoreSubmitInfo waitInfo = vkinit::semaphore_submit_info(VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT_KHR, get_current_frame()._swapchainSemaphore);
	VkSemaphoreSubmitInfo signalInfo = vkinit::semaphore_submit_info(VK_PIPELINE_STAGE_2_ALL_GRAPHICS_BIT, get_current_frame()._renderSemaphore);

	VkSubmitInfo2 submit = vkinit::submit_info(&cmdinfo, &signalInfo, &waitInfo);

	//submit command buffer to the queue and execute it.
	// _renderFence will now block until the graphic commands finish execution
	VK_CHECK(vkQueueSubmit2(_graphicsQueue, 1, &submit, get_current_frame()._renderFence));



	//prepare present
	// this will put the image we just rendered to into the visible window.
	// we want to wait on the _renderSemaphore for that, 
	// as its necessary that drawing commands have finished before the image is displayed to the user
	VkPresentInfoKHR presentInfo = vkinit::present_info();

	presentInfo.pSwapchains = &_swapchain;
	presentInfo.swapchainCount = 1;

	presentInfo.pWaitSemaphores = &get_current_frame()._renderSemaphore;
	presentInfo.waitSemaphoreCount = 1;

	presentInfo.pImageIndices = &swapchainImageIndex;

	VkResult presentResult = vkQueuePresentKHR(_graphicsQueue, &presentInfo);
	if (e == VK_ERROR_OUT_OF_DATE_KHR) {
		resize_requested = true;
		return;
	}
	//increase the number of frames drawn
	_frameNumber++;
}
void VulkanEngine::resize_swapchain()
{
	vkDeviceWaitIdle(_device);

	destroy_swapchain();

	int w, h;
	SDL_GetWindowSize(_window, &w, &h);
	_windowExtent.width = w;
	_windowExtent.height = h;

	create_swapchain(_windowExtent.width, _windowExtent.height);

	resize_requested = false;
}

void VulkanEngine::loadSavedHitbox(GameObject* go)
{
	go->_vOBB.clear();
	for (auto& entry : savedHitboxData[go->m_id]) // get the hitboxes from the map of saved ones.
	{
		go->_vOBB.push_back(OBB(entry.second, entry.first)); // for each of them add to the list of hitboxes for taht gameobject
	}
	go->Update();
}

void VulkanEngine::run()
{
	SDL_Event e;
	bool bQuit = false;
	bool _LockMouse{ false };
	SDL_SetRelativeMouseMode(SDL_TRUE); // Lock the mouse position and hide cursor
	//main loop
	float spherescale = 0.5f;
	while (!bQuit)
	{
		auto start = std::chrono::system_clock::now();

		//Handle events on queue
		while (SDL_PollEvent(&e) != 0)
		{
			// Keyboard input
			if (e.type == SDL_KEYDOWN) // A key is pressed
			{
				if (e.key.keysym.sym == SDLK_ESCAPE) // Toggle Lock the mouse when g is pressed
				{
					stopRendering = true;
					bQuit = true;
					ImGui::EndFrame();
				}
				if (e.key.keysym.sym == SDLK_g) // Toggle Lock the mouse when g is pressed
				{
					(_LockMouse) ? SDL_SetRelativeMouseMode(SDL_TRUE) : SDL_SetRelativeMouseMode(SDL_FALSE);
					_LockMouse = !_LockMouse;
				}
				if (e.key.keysym.sym == SDLK_f) // Toggle Lock the mouse when g is pressed
				{
					sceneData.Torch.Position.w *= -1;
				}
				if (e.key.keysym.sym == SDLK_v) // Toggle Lock the mouse when g is pressed
				{
					(mainCamera.bNoclip) ? mainCamera.bNoclip = false : mainCamera.bNoclip = true;
				}

				//if (e.key.keysym.sym == SDLK_CAPSLOCK) // Toggle Lock the mouse when g is pressed
				//{
				//	debugwindow = !debugwindow;
				//}
			}
			//everything else
			auto end = std::chrono::system_clock::now();
			auto elapsed = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
			stats.frametime = elapsed.count();
			stats.TotalTimeInSeconds += std::chrono::duration_cast<std::chrono::seconds>(elapsed).count();
			stats.FPS = (int)(stats.TotalTimeInSeconds / _frameNumber);

			//close the window when user alt-f4s or clicks the X button			
			if (e.type == SDL_QUIT) bQuit = true;

			mainCamera.processSDLEvents(e, _LockMouse);

			if (e.type == SDL_WINDOWEVENT) {

				if (e.window.event == SDL_WINDOWEVENT_MINIMIZED) {
					stopRendering = true;
				}
				if (e.window.event == SDL_WINDOWEVENT_RESTORED) {
					stopRendering = false;
				}
				if (resize_requested)
				{
					resize_swapchain();
				}


			}

			//send SDL event to imgui for handling
			ImGui_ImplSDL2_ProcessEvent(&e);

		}
		//do not draw if we are minimized
		if (stopRendering)
		{
			//throttle the speed to avoid the endless spinning
			std::this_thread::sleep_for(std::chrono::milliseconds(100));
			continue;
		}


		// imgui new frame
		ImGui_ImplVulkan_NewFrame();
		ImGui_ImplSDL2_NewFrame();
		ImGui::NewFrame();
		if (debugwindow)
		{
			// Camera Window
			{
				ImGui::Begin("Camera");
				ImGui::SetWindowPos({ 150, 50 });
				float* mv = &mainCamera.moveSpeed;
				float* fov = &mainCamera.FOV;
				ImGui::Text("Pos - (%.3f, %.3f, %.3f)", mainCamera.Position.x, mainCamera.Position.y, mainCamera.Position.z);
				ImGui::Text("pitch/yaw - (%.3f, %.3f, %.3f)", mainCamera.pitch, mainCamera.yaw);

				ImGui::DragFloat("MoveSpeed", mv, 0.05f, -FLT_MAX, FLT_MAX, "%.3f");
				ImGui::DragFloat("FOV", fov, 0.05f, -FLT_MAX, FLT_MAX, "%.3f");
				ImGui::End();
			}
			// Stats Window
			{
				ImGui::Begin("Stats");

				ImGui::Text("FPS %i", stats.FPS);
				ImGui::Text("frametime %f ms", stats.frametime);
				ImGui::Text("draw time %f ms", stats.mesh_draw_time);
				ImGui::Text("Collisions Detected %i", stats.Collisions);
				ImGui::Text("Collision Resolve Time %f ms", stats.col_resolve_time);
				ImGui::Text("update time %f ms", stats.scene_update_time);
				ImGui::Text("triangles %i", stats.triangle_count);
				ImGui::Text("draws %i", stats.drawcall_count);

				ImGui::End();

			}
			// saver window
			{
				ImGui::Begin("Scene saver");
				if (ImGui::Button("New"))
				{
					_vGameObjects.clear();
					createDebugSpheres();

				}

				if (ImGui::BeginTabBar("Scene"))
				{

					if (ImGui::BeginTabItem("Open"))
					{
						//_vGameObjects.clear();

						std::string path = "../json";

						for (auto& entry : std::filesystem::directory_iterator(path))
						{
							if (ImGui::Button(entry.path().filename().string().c_str()))
							{
								_vGameObjects.clear();
								createDebugSpheres();
								read_sceneJson(this, entry.path().string().c_str());
							}
						}

						ImGui::EndTabItem();
					}


					if (ImGui::BeginTabItem("Save"))
					{
						static char password[64] = "test";
						ImGui::InputText("filename", password, IM_ARRAYSIZE(password));
						if (ImGui::Button("Save as"))
						{
							std::string path = "..\\json\\";

							path += password;
							path += ".json";

							write_sceneJson(this, path);
						}
						ImGui::EndTabItem();
					}
					ImGui::EndTabBar();
				}
				ImGui::End();
			}
			float sunDir[4] = { 0.f, 0.f, 0.f, 0.f };
			float sunClr[4] = { 0.f, 0.f, 0.f};
			
			float plPos[3] = { 0.f, 0.f, 0.f };
			float amb[4] = { 0.f, 0.f, 0.f, 0.f};
			if (ImGui::Begin("Shader Data"))
			{
				ComputeEffect& selected = backgroundEffects[currentBackgroundEffect];

				ImGui::Text("Selected Compute Background Effect: ", selected.name);

				ImGui::SliderInt("Effect Index", &currentBackgroundEffect, 0, backgroundEffects.size() - 1);


				/*ImGui::DragFloat4("data1", (float*)&selected.data.data1, 0.05f, -FLT_MAX, FLT_MAX, "%.3f");
				ImGui::DragFloat4("data2", (float*)&selected.data.data2, 0.05f, -FLT_MAX, FLT_MAX, "%.3f");
				ImGui::DragFloat4("data3", (float*)&selected.data.data3, 0.05f, -FLT_MAX, FLT_MAX, "%.3f");
				ImGui::DragFloat4("data4", (float*)&selected.data.data4, 0.05f, -FLT_MAX, FLT_MAX, "%.3f");*/

				ImGui::Text("Lighting Editor");

				// Directional Light / Sun Editor
				{
					sunClr[0] = sceneData.Sun.sClr.x;
					sunClr[1] = sceneData.Sun.sClr.y;
					sunClr[2] = sceneData.Sun.sClr.z;

					sunDir[0] = sceneData.Sun.sDir.x;
					sunDir[1] = sceneData.Sun.sDir.y;
					sunDir[2] = sceneData.Sun.sDir.z;
					sunDir[3] = sceneData.Sun.sDir.w;

					ImGui::Text("Sun / Directional Light");

					ImGui::DragFloat3("Sun Direction", sunDir, 0.005f, -FLT_MAX, FLT_MAX, "%.4f");
					ImGui::DragFloat("Sun Power", &sunDir[3], 0.005f, -FLT_MAX, FLT_MAX, "%.4f");
					ImGui::DragFloat3("Sun Colour", sunClr, 0.005f, -FLT_MAX, FLT_MAX, "%.4f");

					sceneData.Sun.sClr.x = sunClr[0];
					sceneData.Sun.sClr.y = sunClr[1];
					sceneData.Sun.sClr.z = sunClr[2];

					sceneData.Sun.sDir.x = sunDir[0];
					sceneData.Sun.sDir.y = sunDir[1];
					sceneData.Sun.sDir.z = sunDir[2];
					sceneData.Sun.sDir.w = sunDir[3];
				}

				// Point light stuff
				{
					int count = 0;
					for (PointLight& pl : sceneData.pLights)
					{
						count += 1;
						if (ImGui::TreeNode(std::string("PointLight #" + std::to_string(count)).c_str()))
						{
							if (ImGui::Button("Put light here"))
							{
								pl.Position = v4(mainCamera.Position, pl.Position.w);
							}
							amb[0] = pl.Ambient_Colour.x;
							amb[1] = pl.Ambient_Colour.y;
							amb[2] = pl.Ambient_Colour.z;
							amb[3] = pl.Ambient_Colour.w;

							plPos[0] = pl.Position.x;
							plPos[1] = pl.Position.y;
							plPos[2] = pl.Position.z;
							//sunDir[3] = sceneData.sDir.w;

							ImGui::Text("Point Lights");
							ImGui::DragFloat3("Position", plPos, 0.005f, -FLT_MAX, FLT_MAX, "%.4f");
							ImGui::DragFloat4("Colour", amb, 0.005f, -FLT_MAX, FLT_MAX, "%.4f");

							pl.Ambient_Colour.x = amb[0];
							pl.Ambient_Colour.y = amb[1];
							pl.Ambient_Colour.z = amb[2];
							pl.Ambient_Colour.w = amb[3];

							pl.Position.x = plPos[0];
							pl.Position.y = plPos[1];
							pl.Position.z = plPos[2];
							//sceneData.sDir.w = sunDir[3];
							ImGui::TreePop();
						}
					}

				}
				ImGui::End();
			}
			float dpos[3] = { 0, 0, 0 };
			float drot[3] = { 0, 0, 0 };
			float dSca[3] = { 0, 0, 0 };

			if (ImGui::Begin("Editor Window"))
			{

				ImGui::SliderFloat("Render Scale", &renderScale, 0.3f, 1.f);
				if (ImGui::TreeNode("Scene Hierarchy"))
				{

					for (auto& Scenes : loadedGLTFs)
					{
						if (ImGui::TreeNode(Scenes.first.c_str()))
						{
							if (ImGui::TreeNode("Top Nodes"))
							{
								int count = 0;
								for (auto& m : Scenes.second->topNodes)
								{
									++count;
									std::string c = "Node - ";
									c += count;

									if (ImGui::TreeNode(c.c_str()))
									{
										ImGui::Text("Transform Editor");
										dpos[0] = m->vTrans.x;
										dpos[1] = m->vTrans.y;
										dpos[2] = m->vTrans.z;

										drot[0] = m->qRot.x;
										drot[1] = m->qRot.y;
										drot[2] = m->qRot.z;

										dSca[0] = m->vScale.x;
										dSca[1] = m->vScale.y;
										dSca[2] = m->vScale.z;


										ImGui::DragFloat3("Position", dpos, 0.05f, -FLT_MAX, FLT_MAX, "%.3f");

										ImGui::DragFloat3("Rotation", drot, 0.05f, -FLT_MAX, FLT_MAX, "%.3f");

										ImGui::DragFloat3("Scale", dSca, 0.05f, -FLT_MAX, FLT_MAX, "%.3f");

										m->vTrans.x = dpos[0];
										m->vTrans.y = dpos[1];
										m->vTrans.z = dpos[2];

										m->qRot.x = drot[0];
										m->qRot.y = drot[1];
										m->qRot.z = drot[2];
										m->qRot.w = 1;
										m->vScale.x = dSca[0];
										m->vScale.y = dSca[1];
										m->vScale.z = dSca[2];

										m4 mMat = glm::translate(v3(dpos[0], dpos[1], dpos[2])) * glm::rotate(m->qRot.x, v3(1, 0, 0)) * glm::rotate(m->qRot.y, v3(0, 1, 0)) * glm::rotate(m->qRot.z, v3(0, 0, 1));
										mMat *= glm::scale(m->vScale);
										m->refreshTransform(mMat);
										ImGui::TreePop();
									}
								}
								ImGui::TreePop();

							}
							if (ImGui::TreeNode("Meshes"))
							{
								for (auto& m : Scenes.second->meshes)
								{
									if (ImGui::TreeNode(m.first.c_str()))
									{
										if (ImGui::TreeNode("Surfaces"))
										{
											for (auto& surface : m.second->surfaces)
											{
												ImGui::Text("Surface Count %d", surface.count);
											}

											ImGui::TreePop();
										}


										ImGui::TreePop();
									}


								}
								ImGui::TreePop();
							}
							if (ImGui::TreeNode("Materials"))
							{
								for (auto& m : Scenes.second->materials)
								{
									if (ImGui::TreeNode(m.first.c_str()))
									{
										(m.second->data.passType == MaterialPass::MainColor) ? ImGui::Text("passType Main") : ImGui::Text("passType Transparent");

										ImGui::TreePop();
									}

								}
								ImGui::TreePop();
							}
							if (ImGui::TreeNode("Images"))
							{
								for (auto& m : Scenes.second->images)
								{
									if (ImGui::TreeNode(m.first.c_str()))
									{
										ImTextureID my_tex_id = m.second.image;
										float my_tex_w = (float)m.second.imageExtent.width;
										float my_tex_h = (float)m.second.imageExtent.height;

										ImVec2 uv0 = ImVec2(0, 0);
										ImVec2 uv1 = ImVec2(my_tex_w, my_tex_h);
										ImGui::Image(my_tex_id, ImVec2(100, 100), ImVec2(0, 1), ImVec2(1, 0));
										ImGui::TreePop();

									}

								}
								ImGui::TreePop();

							}
							ImGui::TreePop();
						}
					}
					ImGui::TreePop();

				}
				if (ImGui::TreeNode("GameObjects"))
				{
					if (ImGui::Button("Add GameObject"))
					{
						GameObject go;
						go.m_id = "Cube";
						go._pGLTF = loadedGLTFs[go.m_id];
						go.pos = mainCamera.Position;
						go.pos.z += 15.f;
						//go.rot = { 0, 0, 0, 0 };
						go.scale = v3(1);
						go._modelMat = m4(1);
						go.Name = "TestObject - " + std::to_string(_vGameObjects.size() + 1) + go.m_id;
						go._vOBB.clear();
						for (auto& m : savedHitboxData[go.m_id])
						{
							go._vOBB.push_back(OBB(m.second, m.first));
						}
						_vGameObjects.push_back(go);
					}
					else
					{
						// Start at 8 to ignore hitbox spheres
						for (int y = 8; y < _vGameObjects.size(); ++y)
						{
							auto& GO = _vGameObjects[y];
							if (GO.followPlight)
							{
								//GO.pos = sceneData.pointLightPos;

							}
							if (ImGui::TreeNode(std::string(GO.Name + " (" + GO.m_id + ")" ).c_str()))
							{
								if (ImGui::Button("Attach to PLight"))
								{
									GO._transform.position = sceneData.pLights[0].Position;
									//GO.followPlight = true;
								} ImGui::SameLine();
								if (ImGui::Button("Copy GameObject"))
								{
									GameObject go = GO;
									go.Name = GO.Name + "_Copy";
									_vGameObjects.push_back(go);
								} ImGui::SameLine();
								if (ImGui::Button("Load Saved Hitbox"))
								{
									loadSavedHitbox(&GO);
								}
								ImGui::Checkbox("Simulate", &GO._simulate);
								ImGui::Checkbox("Static", &GO._static);



								if (ImGui::TreeNode("Model & Texture Editor"))
								{

									if (ImGui::BeginListBox("Models"))
									{
										static int item_current_idx = 0;
										int cnt = 0;
										for (auto& gltf : loadedGLTFs)
										{
											++cnt;
											const bool is_selected = (item_current_idx == cnt);
											if (ImGui::Selectable(gltf.first.c_str(), is_selected))
												item_current_idx = cnt;

											// Set the initial focus when opening the combo (scrolling + keyboard navigation focus)
											if (is_selected)
											{
												ImGui::SetItemDefaultFocus();
												item_current_idx = cnt;
												GO._pGLTF = gltf.second;
												GO.m_id = gltf.first;
											}

										}

										ImGui::EndListBox();
									}




									ImGui::TreePop();
								}

								if (ImGui::TreeNode("Transform Editor"))
								{
									dpos[0] = GO._transform.position.x;
									dpos[1] = GO._transform.position.y;
									dpos[2] = GO._transform.position.z;

									drot[0] = GO._transform.rotation.x;
									drot[1] = GO._transform.rotation.y;
									drot[2] = GO._transform.rotation.z;

									dSca[0] = GO._transform.scale.x;
									dSca[1] = GO._transform.scale.y;
									dSca[2] = GO._transform.scale.z;

									ImGui::DragFloat3("Position", dpos, 0.05f, -FLT_MAX, FLT_MAX, "%.3f");

									ImGui::DragFloat3("Rotation", drot, 0.05f, -FLT_MAX, FLT_MAX, "%.3f");

									ImGui::DragFloat3("Scale", dSca, 0.05f, -FLT_MAX, FLT_MAX, "%.3f");

									GO._transform.position.x = dpos[0];
									GO._transform.position.y = dpos[1];
									GO._transform.position.z = dpos[2];

									GO._transform.rotation.x = drot[0];
									GO._transform.rotation.y = drot[1];
									GO._transform.rotation.z = drot[2];
									GO._transform.scale.x = dSca[0];
									GO._transform.scale.y = dSca[1];
									GO._transform.scale.z = dSca[2];

									GO.updateModelMat();

									ImGui::TreePop();

								}

								if (ImGui::TreeNode("Velocity Editor"))
								{
									dpos[0] = GO.velocity.x;
									dpos[1] = GO.velocity.y;
									dpos[2] = GO.velocity.z;

									dSca[0] = GO.rot_velocity.x;
									dSca[1] = GO.rot_velocity.y;
									dSca[2] = GO.rot_velocity.z;

									ImGui::DragFloat3("Velocity", dpos, 0.005f, -FLT_MAX, FLT_MAX, "%.3f");

									ImGui::DragFloat3("Rotational Velocity", dSca, 0.005f, -FLT_MAX, FLT_MAX, "%.3f");
									GO.velocity.x = dpos[0];
									GO.velocity.y = dpos[1];
									GO.velocity.z = dpos[2];

									GO.rot_velocity.x = dSca[0];
									GO.rot_velocity.y = dSca[1];
									GO.rot_velocity.z = dSca[2];

									GO.Update();

									ImGui::TreePop();

								}
								
								if (ImGui::TreeNode("Hitbox Editor"))
								{
									if (ImGui::Button("Add Another Hitbox"))
									{
										GO._vOBB.push_back(OBB());

									}
									ImGui::SameLine();
									if (ImGui::Button("Save Hitboxes"))
									{

										std::string path = "..\\json\\";

										path += "hitbox_sizes";
										path += ".json";
										write_hitboxJson(this, path);
									}
									else
									{
										if (GO._vOBB.size() == 1)
										{
											dpos[0] = GO._vOBB[0].e.x;
											dpos[1] = GO._vOBB[0].e.y;
											dpos[2] = GO._vOBB[0].e.z;

											dSca[0] = GO._vOBB[0].offset.x;
											dSca[1] = GO._vOBB[0].offset.y;
											dSca[2] = GO._vOBB[0].offset.z;

											ImGui::DragFloat3("Extents", dpos, 0.005f, -FLT_MAX, FLT_MAX, "%.3f");

											ImGui::DragFloat3("Offset", dSca, 0.005f, -FLT_MAX, FLT_MAX, "%.3f");

											ImGui::DragFloat("Sphere Scale", &spherescale, 0.05, 0, 100, "%.3f");
											GO._vOBB[0].e.x = dpos[0];
											GO._vOBB[0].e.y = dpos[1];
											GO._vOBB[0].e.z = dpos[2];

											GO._vOBB[0].offset.x = dSca[0];
											GO._vOBB[0].offset.y = dSca[1];
											GO._vOBB[0].offset.z = dSca[2];
											auto vert_positions = GO._vOBB[0].m_coords;
											for (int i = 0; i < 8; ++i)
											{
												_vGameObjects[i]._transform.position = GO._vOBB[0].m_coords[i];
												_vGameObjects[i]._transform.scale = v3(spherescale);
												_vGameObjects[i].updateModelMat();
												_vGameObjects[i]._pGLTF = loadedGLTFs["Sphere"];

											}
											GO.Update();
										}
										else
										{
											for (int n = 0; n < GO._vOBB.size(); ++n)
											{
												std::string nam = "Hitbox - " + std::to_string(n);
												if (ImGui::TreeNode(nam.c_str()))
												{
													if (ImGui::Button("Remove Hitbox"))
													{
														GO._vOBB.erase(GO._vOBB.begin() + n);
													}
													else
													{
													
														
														dpos[0] = GO._vOBB[n].e.x;
														dpos[1] = GO._vOBB[n].e.y;
														dpos[2] = GO._vOBB[n].e.z;

														dSca[0] = GO._vOBB[n].offset.x;
														dSca[1] = GO._vOBB[n].offset.y;
														dSca[2] = GO._vOBB[n].offset.z;

														ImGui::DragFloat3("Extents", dpos, 0.005f, -FLT_MAX, FLT_MAX, "%.3f");

														ImGui::DragFloat3("Offset", dSca, 0.005f, -FLT_MAX, FLT_MAX, "%.3f");

														ImGui::DragFloat("Sphere Scale", &spherescale, 0.05, 0, 100, "%.3f");
														GO._vOBB[n].e.x = dpos[0];
														GO._vOBB[n].e.y = dpos[1];
														GO._vOBB[n].e.z = dpos[2];

														GO._vOBB[n].offset.x = dSca[0];
														GO._vOBB[n].offset.y = dSca[1];
														GO._vOBB[n].offset.z = dSca[2];
														auto vert_positions = GO._vOBB[n].m_coords;
														for (int i = 0; i < 8; ++i)
														{
															_vGameObjects[i]._transform.position = GO._vOBB[n].m_coords[i];
															_vGameObjects[i]._transform.scale = v3(spherescale);
															_vGameObjects[i].updateModelMat();
															_vGameObjects[i]._pGLTF = loadedGLTFs["Sphere"];

														}
														GO.Update();
													}
													ImGui::TreePop();

												}

											}

										}

									}
									ImGui::TreePop();
								}
								ImGui::TreePop();

							}

						}

						

					}

					ImGui::TreePop();


				}
				ImGui::End();
			}
			ImGui::Render();

			mainCamera.update();
			sceneData.Torch.Position = v4(mainCamera.Position, 1);
			sceneData.Torch.Direction = v4(mainCamera.forward_dir(), 1);
			//our draw function
			update_scene();

			draw();
			auto end = std::chrono::system_clock::now();
			auto elapsed = std::chrono::duration_cast<std::chrono::microseconds>(end - start);

			stats.frametime = elapsed.count() / 1000.f;
		}
	}
}
float move = 0.5f;
void VulkanEngine::update_scene()
{
	
	auto start = std::chrono::system_clock::now();
	mainDrawContext.OpaqueSurfaces.clear();
	

	if (sceneData.pLights[0].Position.x == 25 || sceneData.pLights[0].Position.x == -25)
	{
		move *= -1.f;
	}
	
	sceneData.pLights[0].Position.x += move;
	
	// Check for collisions
	stats.Collisions = 0;
	if (_vGameObjects.size() > 8)
	{
		// camera vs Gameobject collisions
		for (int i = 8; i < _vGameObjects.size(); ++i)
		{
			GameObject* objL = &_vGameObjects[i];
			if (objL->getCollidable() == false || mainCamera.noclip == true)
			{
				continue; // go to next item
			}
			Simplex s;
			int T_idx = 0; // For knowing which tetrahedron inside the obb the collision was detected on
			for (auto& lOBB : objL->_vOBB)
			{
				if (GJK::GJK(lOBB, mainCamera._hitBox, s, T_idx))
				{

					stats.Collisions += 1;

					CollisionInfo colinfo = EPA(s, &lOBB, &mainCamera._hitBox, T_idx);

					if (colinfo.penDepth < 0.03)
					{
						colinfo.penDepth = 0.00f;
					}
					if (std::isnan(colinfo.penDepth))
					{
						colinfo.penDepth = 0;
					}
					int biggestIdx = 0;
					float max = -FLT_MAX;
					for (int y = 0; y < 3; ++y)
					{
						if (std::isnan(colinfo.Normal[y]))
						{
							colinfo.Normal[y] = 0;
							break;
						}
						colinfo.Normal[y] = fabs(colinfo.Normal[y]);

					}

					v3 DxDir = v3(0);
					DxDir[biggestIdx] = colinfo.Normal[biggestIdx]; // Single element response movement normal pointing from B - A

					v3 Dpos = glm::normalize(colinfo.Normal) * colinfo.penDepth;
					v3 diffVec = glm::normalize(lOBB.c - mainCamera._hitBox.c);

					for (int y = 0; y < 3; ++y)
					{
						if (diffVec[y] < 0) // Obj is either on the right of player or behind
						{
							Dpos[y] *= -1;

						}
					}

					
					
					mainCamera.Position -= (Dpos);
					
					//printf("Colliding with Object - %s  No %d\n", objR->GetModelID().c_str(), i);
				}
			}
		}

		// Game objects vs Game ojbects collisions
		for (int i = 8; i < _vGameObjects.size(); ++i)
		{
			for (int j = 8; j < _vGameObjects.size(); ++j)
			{
				if (i == j)
				{
					continue;
				}
				GameObject* objL = &_vGameObjects[i];
				GameObject* objR = &_vGameObjects[j];

				// Check for AABB Collisions.
				if (objL->getCollidable() == false || objR->getCollidable() == false)
				{
					continue; // go to next item
				}
				if (objL->_static == true && objR->_static == true)
				{
					continue; // go to next item
				}

				Simplex s;
				int T_idx = 0; // For knowing which tetrahedron inside the obb the collision was detected on
				for (auto lOBB : objL->_vOBB)
				{
					for (auto rObb : objR->_vOBB)
					{

						if (GJK::GJK(lOBB, rObb, s, T_idx))
						{

							stats.Collisions += 1;

							CollisionInfo colinfo = EPA(s, &lOBB, &rObb, T_idx);

							if (colinfo.penDepth < 0.03)
							{
								colinfo.penDepth = 0.00f;
							}
							if (std::isnan(colinfo.penDepth))
							{
								colinfo.penDepth = 0;
							}
							int biggestIdx = 0;
							float max = -FLT_MAX;
							for (int y = 0; y < 3; ++y)
							{
								if (std::isnan(colinfo.Normal[y]))
								{
									colinfo.Normal[y] = 0;
									break;
								}
								colinfo.Normal[y] = fabs(colinfo.Normal[y]);

							}

							v3 DxDir = v3(0);
							DxDir[biggestIdx] = colinfo.Normal[biggestIdx]; // Single element response movement normal pointing from B - A

							v3 Dpos = glm::normalize(colinfo.Normal) * colinfo.penDepth;
							v3 diffVec = glm::normalize(lOBB.c - rObb.c);

							for (int y = 0; y < 3; ++y)
							{
								if (diffVec[y] < 0) // Obj is either on the right of player or behind
								{
									Dpos[y] *= -1;

								}
							}

							if (!objL->_static)
							{
								objL->_transform.position += (Dpos * v3(0.5f));
							}
							if (!objR->_static)
							{
								objR->_transform.position -= (Dpos * v3(0.5f));
							}

							objR->Update();
							objL->Update();

							//printf("Colliding with Object - %s  No %d\n", objR->GetModelID().c_str(), i);
						}
					}
				}
			}
		}
	}
	auto end = std::chrono::system_clock::now();
	auto elapsed = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
	stats.col_resolve_time = elapsed.count() / 1000.f;
	
	start = std::chrono::system_clock::now();

	// Update gameobjects
	for (auto& entry : _vGameObjects)
	{

		entry.Update();
		if (entry._pGLTF != nullptr) entry._pGLTF->Draw(entry._modelMat, mainDrawContext);

	}
	// Update rendering matrices
	glm::mat4 view = mainCamera.getViewMat();

	// camera projection
	glm::mat4 projection = glm::perspective(glm::radians(mainCamera.FOV), (float)_windowExtent.width / (float)_windowExtent.height, 10000.f, 0.1f);

	// invert the Y direction on projection matrix so that we are more similar
	// to opengl and gltf axis
	projection[1][1] *= -1;

	// Update GPU push data
	sceneData.view = view;
	sceneData.proj = projection;
	sceneData.viewproj = projection * view;

	 end = std::chrono::system_clock::now();
	 elapsed = std::chrono::duration_cast<std::chrono::microseconds>(end - start);

	stats.scene_update_time = elapsed.count() / 1000.f;
	
}
AllocatedImage VulkanEngine::create_image(VkExtent3D size, VkFormat format, VkImageUsageFlags usage, bool mipmapped)
{
	AllocatedImage newImage;
	newImage.imageFormat = format;
	newImage.imageExtent = size;

	VkImageCreateInfo img_info = vkinit::image_create_info(format, usage, size);
	if (mipmapped) {
		img_info.mipLevels = static_cast<uint32_t>(std::floor(std::log2(std::max(size.width, size.height)))) + 1;
	}

	// always allocate images on dedicated GPU memory
	VmaAllocationCreateInfo allocinfo = {};
	allocinfo.usage = VMA_MEMORY_USAGE_GPU_ONLY;
	allocinfo.requiredFlags = VkMemoryPropertyFlags(VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

	// allocate and create the image
	VK_CHECK(vmaCreateImage(_allocator, &img_info, &allocinfo, &newImage.image, &newImage.allocation, nullptr));

	// if the format is a depth format, we will need to have it use the correct
	// aspect flag
	VkImageAspectFlags aspectFlag = VK_IMAGE_ASPECT_COLOR_BIT;
	if (format == VK_FORMAT_D32_SFLOAT) {
		aspectFlag = VK_IMAGE_ASPECT_DEPTH_BIT;
	}

	// build a image-view for the image
	VkImageViewCreateInfo view_info = vkinit::imageview_create_info(format, newImage.image, aspectFlag);
	view_info.subresourceRange.levelCount = img_info.mipLevels;

	VK_CHECK(vkCreateImageView(_device, &view_info, nullptr, &newImage.imageView));

	return newImage;
}

AllocatedImage VulkanEngine::create_image(void* data, VkExtent3D size, VkFormat format, VkImageUsageFlags usage, bool mipmapped)
{
	size_t data_size = size.depth * size.width * size.height * 4;
	AllocatedBuffer uploadbuffer = create_buffer(data_size, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VMA_MEMORY_USAGE_CPU_TO_GPU);

	memcpy(uploadbuffer.info.pMappedData, data, data_size);

	AllocatedImage new_image = create_image(size, format, usage | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT, mipmapped);

	immediate_submit([&](VkCommandBuffer cmd) {
		vkutil::transition_image(cmd, new_image.image, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);

	VkBufferImageCopy copyRegion = {};
	copyRegion.bufferOffset = 0;
	copyRegion.bufferRowLength = 0;
	copyRegion.bufferImageHeight = 0;

	copyRegion.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	copyRegion.imageSubresource.mipLevel = 0;
	copyRegion.imageSubresource.baseArrayLayer = 0;
	copyRegion.imageSubresource.layerCount = 1;
	copyRegion.imageExtent = size;

	// copy the buffer into the image
	vkCmdCopyBufferToImage(cmd, uploadbuffer.buffer, new_image.image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1,
		&copyRegion);

	if (mipmapped) {
		vkutil::generate_mipmaps(cmd, new_image.image, VkExtent2D{ new_image.imageExtent.width,new_image.imageExtent.height });
	}
	else {
		vkutil::transition_image(cmd, new_image.image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
			VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
	}
	});
	destroy_buffer(uploadbuffer);
	return new_image;
}

void VulkanEngine::destroy_image(const AllocatedImage& img)
{
	vkDestroyImageView(_device, img.imageView, nullptr);
	vmaDestroyImage(_allocator, img.image, img.allocation);
}

AllocatedBuffer VulkanEngine::create_buffer(size_t allocSize, VkBufferUsageFlags usage, VmaMemoryUsage memoryUsage)
{
	// allocate buffer
	VkBufferCreateInfo bufferInfo = { .sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO };
	bufferInfo.pNext = nullptr;
	bufferInfo.size = allocSize;

	bufferInfo.usage = usage;

	VmaAllocationCreateInfo vmaallocInfo = {};
	vmaallocInfo.usage = memoryUsage;
	vmaallocInfo.flags = VMA_ALLOCATION_CREATE_MAPPED_BIT;
	AllocatedBuffer newBuffer;

	// allocate the buffer
	VK_CHECK(vmaCreateBuffer(_allocator, &bufferInfo, &vmaallocInfo, &newBuffer.buffer, &newBuffer.allocation,
		&newBuffer.info));

	return newBuffer;
}
void VulkanEngine::destroy_buffer(const AllocatedBuffer& buffer)
{
	vmaDestroyBuffer(_allocator, buffer.buffer, buffer.allocation);
}

constexpr bool bUseValidationLayers = true;
void VulkanEngine::init_triangle_pipeline()
{
	VkShaderModule triangleFragShader;
	if (!vkutil::load_shader_module("F:/MelloEngine/shaders/colored_trianglePS.spv", _device, &triangleFragShader)) {
		printf("Error when building the triangle fragment shader module\n");
	}
	else {
		printf("Triangle fragment shader succesfully loaded\n");
	}

	VkShaderModule triangleVertexShader;
	if (!vkutil::load_shader_module("F:/MelloEngine/shaders/colored_triangleVS.spv", _device, &triangleVertexShader)) {
		printf("Error when building the triangle vertex shader module\n");
	}
	else {
		printf("Triangle vertex shader succesfully loaded\n");
	}

	//build the pipeline layout that controls the inputs/outputs of the shader
	//we are not using descriptor sets or other systems yet, so no need to use anything other than empty default
	VkPipelineLayoutCreateInfo pipeline_layout_info = vkinit::pipeline_layout_create_info();
	VK_CHECK(vkCreatePipelineLayout(_device, &pipeline_layout_info, nullptr, &_trianglePipelineLayout));


	//Builder
	PipelineBuilder pipelineBuilder;

	//use the triangle layout we created
	pipelineBuilder._pipelineLayout = _trianglePipelineLayout;
	//connecting the vertex and pixel shaders to the pipeline
	pipelineBuilder.set_shaders(triangleVertexShader, triangleFragShader);
	//it will draw triangles
	pipelineBuilder.set_input_topology(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST);
	//filled triangles
	pipelineBuilder.set_polygon_mode(VK_POLYGON_MODE_FILL);
	//no backface culling
	pipelineBuilder.set_cull_mode(VK_CULL_MODE_NONE, VK_FRONT_FACE_CLOCKWISE);
	//no multisampling
	pipelineBuilder.set_multisampling_none();
	//no blending
	pipelineBuilder.enable_blending_additive(); 
	pipelineBuilder.enable_depthtest(true, VK_COMPARE_OP_GREATER_OR_EQUAL);

	//connect the image format we will draw into, from draw image
	pipelineBuilder.set_color_attachment_format(_drawImage.imageFormat);
	pipelineBuilder.set_depth_format(_depthImage.imageFormat);

	//finally build the pipeline
	_trianglePipeline = pipelineBuilder.build_pipeline(_device);

	//clean structures
	vkDestroyShaderModule(_device, triangleFragShader, nullptr);
	vkDestroyShaderModule(_device, triangleVertexShader, nullptr);

	_mainDeletionQueue.push_function([&]() {
		vkDestroyPipelineLayout(_device, _trianglePipelineLayout, nullptr);
	vkDestroyPipeline(_device, _trianglePipeline, nullptr);
	});

}

GPUMeshBuffers VulkanEngine::uploadMesh(std::span<uint32_t> indices, std::span<Vertex> vertices)
{
	const size_t vertexBufferSize = vertices.size() * sizeof(Vertex);
	const size_t indexBufferSize = indices.size() * sizeof(uint32_t);

	GPUMeshBuffers newSurface;

	//create vertex buffer
	newSurface.vertexBuffer = create_buffer(vertexBufferSize, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT,
		VMA_MEMORY_USAGE_GPU_ONLY);

	//find the adress of the vertex buffer
	VkBufferDeviceAddressInfo deviceAdressInfo{ .sType = VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO,.buffer = newSurface.vertexBuffer.buffer };
	newSurface.vertexBufferAddress = vkGetBufferDeviceAddress(_device, &deviceAdressInfo);

	//create index buffer
	newSurface.indexBuffer = create_buffer(indexBufferSize, VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
		VMA_MEMORY_USAGE_GPU_ONLY);

	AllocatedBuffer staging = create_buffer(vertexBufferSize + indexBufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VMA_MEMORY_USAGE_CPU_ONLY);

	void* data = staging.allocation->GetMappedData();

	// copy vertex buffer
	memcpy(data, vertices.data(), vertexBufferSize);
	// copy index buffer
	memcpy((char*)data + vertexBufferSize, indices.data(), indexBufferSize);

	immediate_submit([&](VkCommandBuffer cmd) {
		VkBufferCopy vertexCopy{ 0 };
	vertexCopy.dstOffset = 0;
	vertexCopy.srcOffset = 0;
	vertexCopy.size = vertexBufferSize;

	vkCmdCopyBuffer(cmd, staging.buffer, newSurface.vertexBuffer.buffer, 1, &vertexCopy);

	VkBufferCopy indexCopy{ 0 };
	indexCopy.dstOffset = 0;
	indexCopy.srcOffset = vertexBufferSize;
	indexCopy.size = indexBufferSize;

	vkCmdCopyBuffer(cmd, staging.buffer, newSurface.indexBuffer.buffer, 1, &indexCopy);
	});

	destroy_buffer(staging);

	return newSurface;
}

void VulkanEngine::draw_geometry(VkCommandBuffer cmd)
{
	//reset counters
	stats.drawcall_count = 0;
	stats.triangle_count = 0;
	//begin clock
	auto start = std::chrono::system_clock::now();


	std::vector<uint32_t> opaque_draws;
	opaque_draws.reserve(mainDrawContext.OpaqueSurfaces.size());

	for (int i = 0; i < mainDrawContext.OpaqueSurfaces.size(); i++) {
		if (is_visible(mainDrawContext.OpaqueSurfaces[i], sceneData.viewproj)) {
			opaque_draws.push_back(i);
		}
	}

	// sort the opaque surfaces by material and mesh
	std::sort(opaque_draws.begin(), opaque_draws.end(), [&](const auto& iA, const auto& iB) {
		const RenderObject& A = mainDrawContext.OpaqueSurfaces[iA];
	const RenderObject& B = mainDrawContext.OpaqueSurfaces[iB];
	if (A.material == B.material) {
		return A.indexBuffer < B.indexBuffer;
	}
	else {
		return A.material < B.material;
	}
	});

	//allocate a new uniform buffer for the scene data
	AllocatedBuffer gpuSceneDataBuffer = create_buffer(sizeof(GPUSceneData), VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VMA_MEMORY_USAGE_CPU_TO_GPU);

	//add it to the deletion queue of this frame so it gets deleted once its been used
	get_current_frame()._deletionQueue.push_function([=, this]() {
		destroy_buffer(gpuSceneDataBuffer);
	});

	//write the buffer
	GPUSceneData* sceneUniformData = (GPUSceneData*)gpuSceneDataBuffer.allocation->GetMappedData();
	*sceneUniformData = sceneData;

	//create a descriptor set that binds that buffer and update it
	VkDescriptorSet globalDescriptor = get_current_frame()._frameDescriptors.allocate(_device, _gpuSceneDataDescriptorLayout);

	DescriptorWriter writer;
	writer.write_buffer(0, gpuSceneDataBuffer.buffer, sizeof(GPUSceneData), 0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER);
	writer.update_set(_device, globalDescriptor);

	MaterialPipeline* lastPipeline = nullptr;
	MaterialInstance* lastMaterial = nullptr;
	VkBuffer lastIndexBuffer = VK_NULL_HANDLE;

	auto draw = [&](const RenderObject& r)
	{
		if (r.material != lastMaterial) {
			lastMaterial = r.material;
			if (r.material->pipeline != lastPipeline) {

				lastPipeline = r.material->pipeline;
				vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, r.material->pipeline->pipeline);
				vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, r.material->pipeline->layout, 0, 1,
					&globalDescriptor, 0, nullptr);

				VkViewport viewport = {};
				viewport.x = 0;
				viewport.y = 0;
				viewport.width = (float)_windowExtent.width;
				viewport.height = (float)_windowExtent.height;
				viewport.minDepth = 0.f;
				viewport.maxDepth = 1.f;

				vkCmdSetViewport(cmd, 0, 1, &viewport);

				VkRect2D scissor = {};
				scissor.offset.x = 0;
				scissor.offset.y = 0;
				scissor.extent.width = _windowExtent.width;
				scissor.extent.height = _windowExtent.height;

				vkCmdSetScissor(cmd, 0, 1, &scissor);
			}

			vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, r.material->pipeline->layout, 1, 1,
				&r.material->materialSet, 0, nullptr);
		}
		if (r.indexBuffer != lastIndexBuffer) {
			lastIndexBuffer = r.indexBuffer;
			vkCmdBindIndexBuffer(cmd, r.indexBuffer, 0, VK_INDEX_TYPE_UINT32);
		}
		// calculate final mesh matrix
		GPUDrawPushConstants push_constants;
		push_constants.worldMatrix = r.transform;
		//push_constants.modelMatrix = r.transform;
		push_constants.vertexBuffer = r.vertexBufferAddress;

		vkCmdPushConstants(cmd, r.material->pipeline->layout, VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(GPUDrawPushConstants), &push_constants);

		stats.drawcall_count++;
		stats.triangle_count += r.indexCount / 3;
		//add counters for triangles and draws
		stats.triangle_count += r.indexCount / 3;		
		
		vkCmdDrawIndexed(cmd, r.indexCount, 1, r.firstIndex, 0, 0);

	};

	

	for (auto& r : opaque_draws) 
	{
		draw(mainDrawContext.OpaqueSurfaces[r]);
	}

	for (auto& r : mainDrawContext.TransparentSurfaces) {
		draw(r);
	}

	// we delete the draw commands now that we processed them
	mainDrawContext.OpaqueSurfaces.clear();
	mainDrawContext.TransparentSurfaces.clear();
	auto end = std::chrono::system_clock::now();

	//convert to microseconds (integer), and then come back to miliseconds
	auto elapsed = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
	stats.mesh_draw_time = elapsed.count() / 1000.f;
}
void VulkanEngine::init_mesh_pipeline()
{
	// Load shaders
	VkShaderModule meshFragShader;
	if (!vkutil::load_shader_module("F:/MelloEngine/shaders/mesh.frag.spv", _device, &meshFragShader)) {
		printf("Error when building the Mesh fragment shader module");
	}
	else {
		printf("Mesh fragment shader succesfully loaded");
	}

	VkShaderModule meshVertexShader;
	if (!vkutil::load_shader_module("F:/MelloEngine/shaders/mesh.vert.spv", _device, &meshVertexShader)) {
		printf("Error when building the Mesh vertex shader module");
	}
	else {
		printf("Mesh vertex shader succesfully loaded");
	}


	// Define Push Constnats
	VkPushConstantRange bufferRange{};
	bufferRange.offset = 0;
	bufferRange.size = sizeof(GPUDrawPushConstants);
	bufferRange.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
	// Create Layout info
	VkPipelineLayoutCreateInfo pipeline_layout_info = vkinit::pipeline_layout_create_info();
	pipeline_layout_info.pPushConstantRanges = &bufferRange;
	pipeline_layout_info.pushConstantRangeCount = 1;
	pipeline_layout_info.pSetLayouts = &_singleImageDescriptorLayout;
	pipeline_layout_info.setLayoutCount = 1;
	VK_CHECK(vkCreatePipelineLayout(_device, &pipeline_layout_info, nullptr, &_meshPipelineLayout));



	PipelineBuilder pipelineBuilder;

	//use the triangle layout we created
	pipelineBuilder._pipelineLayout = _meshPipelineLayout;
	//connecting the vertex and pixel shaders to the pipeline
	pipelineBuilder.set_shaders(meshVertexShader, meshFragShader);
	//it will draw triangles
	pipelineBuilder.set_input_topology(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST);
	//filled triangles
	pipelineBuilder.set_polygon_mode(VK_POLYGON_MODE_FILL);
	//no backface culling
	pipelineBuilder.set_cull_mode(VK_CULL_MODE_NONE, VK_FRONT_FACE_CLOCKWISE);
	//no multisampling
	pipelineBuilder.set_multisampling_none();
	//no blending
	pipelineBuilder.enable_blending_additive();

	pipelineBuilder.enable_depthtest(true, VK_COMPARE_OP_GREATER_OR_EQUAL);

	//connect the image format we will draw into, from draw image
	pipelineBuilder.set_color_attachment_format(_drawImage.imageFormat);
	pipelineBuilder.set_depth_format(_depthImage.imageFormat);

	//finally build the pipeline
	_meshPipeline = pipelineBuilder.build_pipeline(_device);

	//clean structures
	vkDestroyShaderModule(_device, meshFragShader, nullptr);
	vkDestroyShaderModule(_device, meshVertexShader, nullptr);

	_mainDeletionQueue.push_function([&]() {
		vkDestroyPipelineLayout(_device, _meshPipelineLayout, nullptr);
	vkDestroyPipeline(_device, _meshPipeline, nullptr);
	});

}
void VulkanEngine::immediate_submit(std::function<void(VkCommandBuffer cmd)>&& function)
{
	VK_CHECK(vkResetFences(_device, 1, &_immFence));
	VK_CHECK(vkResetCommandBuffer(_immCommandBuffer, 0));

	VkCommandBuffer cmd = _immCommandBuffer;

	VkCommandBufferBeginInfo cmdBeginInfo = vkinit::command_buffer_begin_info(VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT);

	VK_CHECK(vkBeginCommandBuffer(cmd, &cmdBeginInfo));

	function(cmd);

	VK_CHECK(vkEndCommandBuffer(cmd));

	VkCommandBufferSubmitInfo cmdinfo = vkinit::command_buffer_submit_info(cmd);
	VkSubmitInfo2 submit = vkinit::submit_info(&cmdinfo, nullptr, nullptr);

	// submit command buffer to the queue and execute it.
	//  _renderFence will now block until the graphic commands finish execution
	VK_CHECK(vkQueueSubmit2(_graphicsQueue, 1, &submit, _immFence));

	VK_CHECK(vkWaitForFences(_device, 1, &_immFence, true, 9999999999));
}
void VulkanEngine::init_vulkan()
{
	// Create Instance
	vkb::InstanceBuilder builder;

	//make the vulkan instance, with basic debug features
	auto inst_ret = builder.set_app_name("Mello Engine")
		.request_validation_layers(bUseValidationLayers)
		.use_default_debug_messenger()
		.require_api_version(1, 3, 0)
		.build();

	vkb::Instance vkb_inst = inst_ret.value();

	//grab the instance 
	_instance = vkb_inst.instance;
	_debug_messenger = vkb_inst.debug_messenger;
	

	// Physical Device
	// get the surface of the window we opened with SDL
	SDL_Vulkan_CreateSurface(_window, _instance, &_surface);

	VkPhysicalDeviceVulkan13Features features{ .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_3_FEATURES, .pNext = nullptr };
	features.dynamicRendering = true;
	features.synchronization2 = true;
	
	//vulkan 1.2 features
	VkPhysicalDeviceVulkan12Features features12{ .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_2_FEATURES, .pNext = nullptr};
	features12.bufferDeviceAddress = true;
	features12.descriptorIndexing = true;

	//use vkbootstrap to select a gpu. 
	//We want a gpu that can write to the SDL surface and supports vulkan 1.3 with the correct features
	vkb::PhysicalDeviceSelector selector{ vkb_inst };
	vkb::PhysicalDevice physicalDevice = selector
		.set_minimum_version(1, 3)
		.set_required_features_13(features)
		.set_required_features_12(features12)
		.set_surface(_surface)
		.select()
		.value();

	//create the final Vulkan device
	vkb::DeviceBuilder deviceBuilder{ physicalDevice };

	vkb::Device vkbDevice = deviceBuilder.build().value();

	// Get the VkDevice handle used in the rest of a Vulkan application
	_device = vkbDevice.device;
	_chosenGPU = physicalDevice.physical_device;

	// use vkbootstrap to get a Graphics queue
	_graphicsQueue = vkbDevice.get_queue(vkb::QueueType::graphics).value();
	_graphicsQueueFamily = vkbDevice.get_queue_index(vkb::QueueType::graphics).value();

	// initialize the memory allocator
	VmaAllocatorCreateInfo allocatorInfo = {};
	allocatorInfo.physicalDevice = _chosenGPU;
	allocatorInfo.device = _device;
	allocatorInfo.instance = _instance;
	allocatorInfo.flags = VMA_ALLOCATOR_CREATE_BUFFER_DEVICE_ADDRESS_BIT;
	vmaCreateAllocator(&allocatorInfo, &_allocator);

	_mainDeletionQueue.push_function([&]() {
		vmaDestroyAllocator(_allocator);
	});
}

void VulkanEngine::draw_imgui(VkCommandBuffer cmd, VkImageView targetImageView)
{
	VkRenderingAttachmentInfo colorAttachment = vkinit::attachment_info(targetImageView, nullptr, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);
	VkRenderingInfo renderInfo = vkinit::rendering_info(_swapchainExtent, &colorAttachment, nullptr);

	vkCmdBeginRendering(cmd, &renderInfo);
	
	ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(), cmd);

	vkCmdEndRendering(cmd);
}
void VulkanEngine::init_swapchain()
{
	create_swapchain(_windowExtent.width, _windowExtent.height);
	//draw image size will match the window
	VkExtent3D drawImageExtent = {
		_windowExtent.width,
		_windowExtent.height,
		1
	};

	//hardcoding the draw format to 32 bit float
	_drawImage.imageFormat = VK_FORMAT_R16G16B16A16_SFLOAT;
	_drawImage.imageExtent = drawImageExtent;

	VkImageUsageFlags drawImageUsages{};
	drawImageUsages |= VK_IMAGE_USAGE_TRANSFER_SRC_BIT;
	drawImageUsages |= VK_IMAGE_USAGE_TRANSFER_DST_BIT;
	drawImageUsages |= VK_IMAGE_USAGE_STORAGE_BIT;
	drawImageUsages |= VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

	VkImageCreateInfo rimg_info = vkinit::image_create_info(_drawImage.imageFormat, drawImageUsages, drawImageExtent);

	//for the draw image, we want to allocate it from gpu local memory
	VmaAllocationCreateInfo rimg_allocinfo = {};
	rimg_allocinfo.usage = VMA_MEMORY_USAGE_GPU_ONLY;
	rimg_allocinfo.requiredFlags = VkMemoryPropertyFlags(VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

	//allocate and create the image
	vmaCreateImage(_allocator, &rimg_info, &rimg_allocinfo, &_drawImage.image, &_drawImage.allocation, nullptr);

	//build a image-view for the draw image to use for rendering
	VkImageViewCreateInfo rview_info = vkinit::imageview_create_info(_drawImage.imageFormat, _drawImage.image, VK_IMAGE_ASPECT_COLOR_BIT);

	VK_CHECK(vkCreateImageView(_device, &rview_info, nullptr, &_drawImage.imageView));

	// depth
	_depthImage.imageFormat = VK_FORMAT_D32_SFLOAT;
	_depthImage.imageExtent = drawImageExtent;
	VkImageUsageFlags depthImageUsages{};
	depthImageUsages |= VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;

	VkImageCreateInfo dimg_info = vkinit::image_create_info(_depthImage.imageFormat, depthImageUsages, drawImageExtent);

	//allocate and create the image
	vmaCreateImage(_allocator, &dimg_info, &rimg_allocinfo, &_depthImage.image, &_depthImage.allocation, nullptr);

	//build a image-view for the draw image to use for rendering
	VkImageViewCreateInfo dview_info = vkinit::imageview_create_info(_depthImage.imageFormat, _depthImage.image, VK_IMAGE_ASPECT_DEPTH_BIT);

	VK_CHECK(vkCreateImageView(_device, &dview_info, nullptr, &_depthImage.imageView));

	//add to deletion queues
	_mainDeletionQueue.push_function([=]() {
		vkDestroyImageView(_device, _drawImage.imageView, nullptr);
	vmaDestroyImage(_allocator, _drawImage.image, _drawImage.allocation);

	vkDestroyImageView(_device, _depthImage.imageView, nullptr);
	vmaDestroyImage(_allocator, _depthImage.image, _depthImage.allocation);
	});
}
void VulkanEngine::draw_main(VkCommandBuffer cmd)
{
	ComputeEffect& effect = backgroundEffects[currentBackgroundEffect];

	// bind the background compute pipeline
	vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_COMPUTE, effect.pipeline);

	// bind the descriptor set containing the draw image for the compute pipeline
	vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_COMPUTE, _gradientPipelineLayout, 0, 1, &_drawImageDescriptors, 0, nullptr);

	vkCmdPushConstants(cmd, _gradientPipelineLayout, VK_SHADER_STAGE_COMPUTE_BIT, 0, sizeof(ComputePushConstants), &effect.data);
	// execute the compute pipeline dispatch. We are using 16x16 workgroup size so we need to divide by it
	vkCmdDispatch(cmd, std::ceil(_drawExtent.width / 16.0), std::ceil(_drawExtent.height / 16.0), 1);

	//draw the triangle

	VkRenderingAttachmentInfo colorAttachment = vkinit::attachment_info(_drawImage.imageView, nullptr, VK_IMAGE_LAYOUT_GENERAL);
	VkRenderingAttachmentInfo depthAttachment = vkinit::depth_attachment_info(_depthImage.imageView, VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL);

	VkRenderingInfo renderInfo = vkinit::rendering_info(_drawExtent, &colorAttachment, &depthAttachment);

	vkCmdBeginRendering(cmd, &renderInfo);
	auto start = std::chrono::system_clock::now();
	draw_geometry(cmd);

	auto end = std::chrono::system_clock::now();
	auto elapsed = std::chrono::duration_cast<std::chrono::microseconds>(end - start);

	stats.mesh_draw_time = elapsed.count() / 1000.f;

	vkCmdEndRendering(cmd);
}
void VulkanEngine::init_commands()
{
	//create a command pool for commands submitted to the graphics queue.
	//we also want the pool to allow for resetting of individual command buffers
	VkCommandPoolCreateInfo commandPoolInfo = vkinit::command_pool_create_info(_graphicsQueueFamily, VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT);

	for (int i = 0; i < FRAME_OVERLAP; i++) {

		VK_CHECK(vkCreateCommandPool(_device, &commandPoolInfo, nullptr, &_frames[i]._commandPool));

		// allocate the default command buffer that we will use for rendering
		VkCommandBufferAllocateInfo cmdAllocInfo = vkinit::command_buffer_allocate_info(_frames[i]._commandPool, 1);

		VK_CHECK(vkAllocateCommandBuffers(_device, &cmdAllocInfo, &_frames[i]._mainCommandBuffer));
	}
	//create a command pool for commands submitted to the graphics queue.
	//we also want the pool to allow for resetting of individual command buffers

	
	VK_CHECK(vkCreateCommandPool(_device, &commandPoolInfo, nullptr, &_immCommandPool));

	// allocate the command buffer for immediate submits
	VkCommandBufferAllocateInfo cmdAllocInfo = vkinit::command_buffer_allocate_info(_immCommandPool, 1);

	VK_CHECK(vkAllocateCommandBuffers(_device, &cmdAllocInfo, &_immCommandBuffer));

	_mainDeletionQueue.push_function([=]()
	{ 
	vkDestroyCommandPool(_device, _immCommandPool, nullptr);
	}
	);
	
}

void VulkanEngine::init_sync_structures()
{
	//create syncronization structures
	//one fence to control when the gpu has finished rendering the frame,
	//and 2 semaphores to syncronize rendering with swapchain
	//we want the fence to start signalled so we can wait on it on the first frame
	VkFenceCreateInfo fenceCreateInfo = vkinit::fence_create_info(VK_FENCE_CREATE_SIGNALED_BIT);
	VkSemaphoreCreateInfo semaphoreCreateInfo = vkinit::semaphore_create_info(0);

	for (int i = 0; i < FRAME_OVERLAP; i++)
	{
		VK_CHECK(vkCreateFence(_device, &fenceCreateInfo, nullptr, &_frames[i]._renderFence));

		VK_CHECK(vkCreateSemaphore(_device, &semaphoreCreateInfo, nullptr, &_frames[i]._swapchainSemaphore));
		VK_CHECK(vkCreateSemaphore(_device, &semaphoreCreateInfo, nullptr, &_frames[i]._renderSemaphore));
	}

	VK_CHECK(vkCreateFence(_device, &fenceCreateInfo, nullptr, &_immFence));
	_mainDeletionQueue.push_function([=]() { vkDestroyFence(_device, _immFence, nullptr); });

}

void VulkanEngine::create_swapchain(uint32_t width, uint32_t height)
{
	vkb::SwapchainBuilder swapchainBuilder{ _chosenGPU,_device,_surface };

	_swapchainImageFormat = VK_FORMAT_B8G8R8A8_UNORM;

	vkb::Swapchain vkbSwapchain = swapchainBuilder
		//.use_default_format_selection()
		.set_desired_format(VkSurfaceFormatKHR{ _swapchainImageFormat, VK_COLOR_SPACE_SRGB_NONLINEAR_KHR })
		//use vsync present mode
		.set_desired_present_mode(VK_PRESENT_MODE_FIFO_KHR)
		.set_desired_extent(width, height)
		.add_image_usage_flags(VK_IMAGE_USAGE_TRANSFER_DST_BIT)
		.build()
		.value();

	_swapchainExtent = vkbSwapchain.extent;
	//store swapchain and its related images
	_swapchain = vkbSwapchain.swapchain;
	m_swapchainImages = vkbSwapchain.get_images().value();
	m_swapchainImageViews = vkbSwapchain.get_image_views().value();
}

void VulkanEngine::destroy_swapchain()
{
	vkDestroySwapchainKHR(_device, _swapchain, nullptr);

	// destroy swapchain resources
	for (int i = 0; i < m_swapchainImageViews.size(); i++) 
	{

		vkDestroyImageView(_device, m_swapchainImageViews[i], nullptr);
	}
}

void VulkanEngine::init_descriptors()
{
	//create a descriptor pool that will hold 10 sets with 1 image each
	std::vector<DescriptorAllocatorGrowable::PoolSizeRatio> sizes =
	{
		{ VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 1 },
		{ VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1 }

	};

	globalDescriptorAllocator.init(_device, 10, sizes);

	//make the descriptor set layout for our compute draw
	//make the descriptor set layout for our compute draw
	{
		DescriptorLayoutBuilder builder;
		builder.add_binding(0, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE);
		_drawImageDescriptorLayout = builder.build(_device, VK_SHADER_STAGE_COMPUTE_BIT);
	}
	{
		DescriptorLayoutBuilder builder;
		builder.add_binding(0, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER);
		_singleImageDescriptorLayout = builder.build(_device, VK_SHADER_STAGE_FRAGMENT_BIT);
	}
	{
		DescriptorLayoutBuilder builder;
		builder.add_binding(0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER);
		_gpuSceneDataDescriptorLayout = builder.build(_device, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT);
	}
	//allocate a descriptor set for our draw image
	_drawImageDescriptors = globalDescriptorAllocator.allocate(_device, _drawImageDescriptorLayout);

	{
		DescriptorWriter writer;
		writer.write_image(0, _drawImage.imageView, VK_NULL_HANDLE, VK_IMAGE_LAYOUT_GENERAL, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE);

		writer.update_set(_device, _drawImageDescriptors);
	}

	for (int i = 0; i < FRAME_OVERLAP; i++) {
		// create a descriptor pool
		std::vector<DescriptorAllocatorGrowable::PoolSizeRatio> frame_sizes = {
			{ VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 3 },
			{ VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 3 },
			{ VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 3 },
			{ VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 4 },
		};

		_frames[i]._frameDescriptors = DescriptorAllocatorGrowable{};
		_frames[i]._frameDescriptors.init(_device, 1000, frame_sizes);

		_mainDeletionQueue.push_function([&, i]() {
			_frames[i]._frameDescriptors.destroy_pools(_device);
		});
	}
}

void VulkanEngine::init_background_pipelines()
{
	VkPipelineLayoutCreateInfo computeLayout{};
	computeLayout.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
	computeLayout.pNext = nullptr;
	computeLayout.pSetLayouts = &_drawImageDescriptorLayout;
	computeLayout.setLayoutCount = 1;

	VkPushConstantRange pushConstant{};
	pushConstant.offset = 0;
	pushConstant.size = sizeof(ComputePushConstants);
	pushConstant.stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;

	computeLayout.pPushConstantRanges = &pushConstant;
	computeLayout.pushConstantRangeCount = 1;

	VK_CHECK(vkCreatePipelineLayout(_device, &computeLayout, nullptr, &_gradientPipelineLayout));

	//> comp_pipeline_multi
	VkShaderModule gradientShader;
	if (!vkutil::load_shader_module("F:/MelloEngine/shaders/gradient.spv", _device, &gradientShader)) {
		printf("Error when building the compute shader \n");
	}

	VkShaderModule skyShader;
	if (!vkutil::load_shader_module("F:/MelloEngine/shaders/sky.spv", _device, &skyShader)) {
		printf("Error when building the compute shader \n");
	}

	VkPipelineShaderStageCreateInfo stageinfo{};
	stageinfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	stageinfo.pNext = nullptr;
	stageinfo.stage = VK_SHADER_STAGE_COMPUTE_BIT;
	stageinfo.module = gradientShader;
	stageinfo.pName = "main";

	VkComputePipelineCreateInfo computePipelineCreateInfo{};
	computePipelineCreateInfo.sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO;
	computePipelineCreateInfo.pNext = nullptr;
	computePipelineCreateInfo.layout = _gradientPipelineLayout;
	computePipelineCreateInfo.stage = stageinfo;

	ComputeEffect gradient;
	gradient.layout = _gradientPipelineLayout;
	gradient.name = "gradient";
	gradient.data = {};

	//default colors
	gradient.data.data1 = glm::vec4(1, 0, 0, 1);
	gradient.data.data2 = glm::vec4(0, 0, 1, 1);

	VK_CHECK(vkCreateComputePipelines(_device, VK_NULL_HANDLE, 1, &computePipelineCreateInfo, nullptr, &gradient.pipeline));

	//change the shader module only to create the sky shader
	computePipelineCreateInfo.stage.module = skyShader;

	ComputeEffect sky;
	sky.layout = _gradientPipelineLayout;
	sky.name = "sky";
	sky.data = {};
	//default sky parameters
	sky.data.data1 = glm::vec4(0.1, 0.2, 0.4, 0.97);

	VK_CHECK(vkCreateComputePipelines(_device, VK_NULL_HANDLE, 1, &computePipelineCreateInfo, nullptr, &sky.pipeline));

	//add the 2 background effects into the array
	backgroundEffects.push_back(gradient);
	backgroundEffects.push_back(sky);

	//destroy structures properly
	vkDestroyShaderModule(_device, gradientShader, nullptr);
	vkDestroyShaderModule(_device, skyShader, nullptr);
	_mainDeletionQueue.push_function([=]() {
		vkDestroyPipelineLayout(_device, _gradientPipelineLayout, nullptr);
	vkDestroyPipeline(_device, sky.pipeline, nullptr);
	vkDestroyPipeline(_device, gradient.pipeline, nullptr);
	});
}
void VulkanEngine::createDebugSpheres()
{
	//Create Debug Spheres
	for (int x = 0; x < 8; ++x)
	{

		GameObject go;
		go._pGLTF = loadedGLTFs["Sphere"];
		//go.rot = { 0, 0, 0, 0 };
		go.scale = v3(1);
		go._modelMat = m4(1);
		go.Name = "DebugSphere - " + std::to_string(x);
		go.canCollide = false;
		_vGameObjects.push_back(go);
	}
}
void VulkanEngine::init_default_data() 
{
	std::array<Vertex, 4> rect_vertices;

	rect_vertices[0].position = { 0.5,-0.5, 0 };
	rect_vertices[1].position = { 0.5,0.5, 0 };
	rect_vertices[2].position = { -0.5,-0.5, 0 };
	rect_vertices[3].position = { -0.5,0.5, 0 };

	rect_vertices[0].color = { 0,0, 0,1 };
	rect_vertices[1].color = { 0.5,0.5,0.5 ,1 };
	rect_vertices[2].color = { 1,0, 0,1 };
	rect_vertices[3].color = { 0,1, 0,1 };

	rect_vertices[0].uv_x = 1;
	rect_vertices[0].uv_y = 0;
	rect_vertices[1].uv_x = 0;
	rect_vertices[1].uv_y = 0;
	rect_vertices[2].uv_x = 1;
	rect_vertices[2].uv_y = 1;
	rect_vertices[3].uv_x = 0;
	rect_vertices[3].uv_y = 1;

	std::array<uint32_t, 6> rect_indices;

	rect_indices[0] = 0;
	rect_indices[1] = 1;
	rect_indices[2] = 2;

	rect_indices[3] = 2;
	rect_indices[4] = 1;
	rect_indices[5] = 3;

	rectangle = uploadMesh(rect_indices, rect_vertices);

	//3 default textures, white, grey, black. 1 pixel each
	uint32_t white = glm::packUnorm4x8(glm::vec4(1, 1, 1, 1));
	_whiteImage = create_image((void*)&white, VkExtent3D{ 1, 1, 1 }, VK_FORMAT_R8G8B8A8_UNORM,
		VK_IMAGE_USAGE_SAMPLED_BIT);

	uint32_t grey = glm::packUnorm4x8(glm::vec4(0.66f, 0.66f, 0.66f, 1));
	_greyImage = create_image((void*)&grey, VkExtent3D{ 1, 1, 1 }, VK_FORMAT_R8G8B8A8_UNORM,
		VK_IMAGE_USAGE_SAMPLED_BIT);

	uint32_t black = glm::packUnorm4x8(glm::vec4(0, 0, 0, 0));
	_blackImage = create_image((void*)&black, VkExtent3D{ 1, 1, 1 }, VK_FORMAT_R8G8B8A8_UNORM,
		VK_IMAGE_USAGE_SAMPLED_BIT);

	//checkerboard image
	uint32_t magenta = glm::packUnorm4x8(glm::vec4(1, 0, 1, 1));
	std::array<uint32_t, 16 * 16 > pixels; //for 16x16 checkerboard texture
	for (int x = 0; x < 16; x++) {
		for (int y = 0; y < 16; y++) {
			pixels[y * 16 + x] = ((x % 2) ^ (y % 2)) ? magenta : black;
		}
	}

	_errorCheckerboardImage = create_image(pixels.data(), VkExtent3D{ 16, 16, 1 }, VK_FORMAT_R8G8B8A8_UNORM,
		VK_IMAGE_USAGE_SAMPLED_BIT);

	VkSamplerCreateInfo sampl = { .sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO };

	sampl.magFilter = VK_FILTER_NEAREST;
	sampl.minFilter = VK_FILTER_NEAREST;

	vkCreateSampler(_device, &sampl, nullptr, &_defaultSamplerNearest);

	sampl.magFilter = VK_FILTER_LINEAR;
	sampl.minFilter = VK_FILTER_LINEAR;
	vkCreateSampler(_device, &sampl, nullptr, &_defaultSamplerLinear);

	createDebugSpheres();
	
}

void VulkanEngine::init_renderables()
{
	
	read_assetDataJson(this, "..\\json\\data.json");


	read_hitboxJson(this, "..\\json\\hitbox_sizes.json");

	
	read_sceneJson(this, "..\\json\\test.json");

}
void VulkanEngine::init_pipelines()
{
	//Compute pipeline
	init_background_pipelines();


	// Graphics pipeline
	init_triangle_pipeline();
	init_mesh_pipeline();

	metalRoughMaterial.build_pipelines(this);




}
void VulkanEngine::init_imgui()
{
	// 1: create descriptor pool for IMGUI
	//  the size of the pool is very oversize, but it's copied from imgui demo
	//  itself.
	VkDescriptorPoolSize pool_sizes[] = { { VK_DESCRIPTOR_TYPE_SAMPLER, 1000 },
		{ VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1000 },
		{ VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, 1000 },
		{ VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 1000 },
		{ VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER, 1000 },
		{ VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER, 1000 },
		{ VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1000 },
		{ VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1000 },
		{ VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, 1000 },
		{ VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC, 1000 },
		{ VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT, 1000 } };

	VkDescriptorPoolCreateInfo pool_info = {};
	pool_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
	pool_info.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;
	pool_info.maxSets = 1000;
	pool_info.poolSizeCount = (uint32_t)std::size(pool_sizes);
	pool_info.pPoolSizes = pool_sizes;

	VkDescriptorPool imguiPool;
	VK_CHECK(vkCreateDescriptorPool(_device, &pool_info, nullptr, &imguiPool));

	// 2: initialize imgui library

	// this initializes the core structures of imgui
	ImGui::CreateContext();

	// this initializes imgui for SDL
	ImGui_ImplSDL2_InitForVulkan(_window);

	// this initializes imgui for Vulkan
	ImGui_ImplVulkan_InitInfo init_info = {};
	init_info.Instance = _instance;
	init_info.PhysicalDevice = _chosenGPU;
	init_info.Device = _device;
	init_info.Queue = _graphicsQueue;
	init_info.DescriptorPool = imguiPool;
	init_info.MinImageCount = 3;
	init_info.ImageCount = 3;
	init_info.UseDynamicRendering = true;
	init_info.ColorAttachmentFormat = _swapchainImageFormat;

	init_info.MSAASamples = VK_SAMPLE_COUNT_1_BIT;

	ImGui_ImplVulkan_Init(&init_info, VK_NULL_HANDLE);

	// execute a gpu command to upload imgui font textures
	immediate_submit([&](VkCommandBuffer cmd) { ImGui_ImplVulkan_CreateFontsTexture(cmd); });

	// clear font textures from cpu data
	ImGui_ImplVulkan_DestroyFontUploadObjects();

	// add the destroy the imgui created structures
	_mainDeletionQueue.push_function([=]() {
		vkDestroyDescriptorPool(_device, imguiPool, nullptr);
	ImGui_ImplVulkan_Shutdown();
	});
}

void GLTFMetallic_Roughness::build_pipelines(VulkanEngine* engine)
{
	VkShaderModule meshFragShader;
	if (!vkutil::load_shader_module("F:/MelloEngine/shaders/mesh.frag.spv", engine->_device, &meshFragShader)) {
		printf("Error when building the triangle fragment shader module\n");
	}

	VkShaderModule meshVertexShader;
	if (!vkutil::load_shader_module("F:/MelloEngine/shaders/mesh.vert.spv", engine->_device, &meshVertexShader)) {
		printf("Error when building the triangle vertex shader module\n");
	}

	VkPushConstantRange matrixRange{};
	matrixRange.offset = 0;
	matrixRange.size = sizeof(GPUDrawPushConstants);
	matrixRange.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;

	DescriptorLayoutBuilder layoutBuilder;
	layoutBuilder.add_binding(0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER);
	layoutBuilder.add_binding(1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER);
	layoutBuilder.add_binding(2, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER);

	materialLayout = layoutBuilder.build(engine->_device, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT);

	VkDescriptorSetLayout layouts[] = { engine->_gpuSceneDataDescriptorLayout,
		materialLayout };

	VkPipelineLayoutCreateInfo mesh_layout_info = vkinit::pipeline_layout_create_info();
	mesh_layout_info.setLayoutCount = 2;
	mesh_layout_info.pSetLayouts = layouts;
	mesh_layout_info.pPushConstantRanges = &matrixRange;
	mesh_layout_info.pushConstantRangeCount = 1;

	VkPipelineLayout newLayout;
	VK_CHECK(vkCreatePipelineLayout(engine->_device, &mesh_layout_info, nullptr, &newLayout));

	opaquePipeline.layout = newLayout;
	transparentPipeline.layout = newLayout;

	// build the stage-create-info for both vertex and fragment stages. This lets
	// the pipeline know the shader modules per stage
	PipelineBuilder pipelineBuilder;
	pipelineBuilder.set_shaders(meshVertexShader, meshFragShader);
	pipelineBuilder.set_input_topology(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST);
	pipelineBuilder.set_polygon_mode(VK_POLYGON_MODE_FILL);
	pipelineBuilder.set_cull_mode(VK_CULL_MODE_NONE, VK_FRONT_FACE_CLOCKWISE);
	pipelineBuilder.set_multisampling_none();
	pipelineBuilder.disable_blending();
	pipelineBuilder.enable_depthtest(true, VK_COMPARE_OP_GREATER_OR_EQUAL);

	//render format
	pipelineBuilder.set_color_attachment_format(engine->_drawImage.imageFormat);
	pipelineBuilder.set_depth_format(engine->_depthImage.imageFormat);

	// use the triangle layout we created
	pipelineBuilder._pipelineLayout = newLayout;

	// finally build the pipeline
	opaquePipeline.pipeline = pipelineBuilder.build_pipeline(engine->_device);

	// create the transparent variant
	pipelineBuilder.enable_blending_additive();

	pipelineBuilder.enable_depthtest(false, VK_COMPARE_OP_GREATER_OR_EQUAL);

	transparentPipeline.pipeline = pipelineBuilder.build_pipeline(engine->_device);

	vkDestroyShaderModule(engine->_device, meshFragShader, nullptr);
	vkDestroyShaderModule(engine->_device, meshVertexShader, nullptr);
}

MaterialInstance GLTFMetallic_Roughness::write_material(VkDevice device, MaterialPass pass, const MaterialResources& resources, DescriptorAllocatorGrowable& descriptorAllocator)
{
	MaterialInstance matData;
	matData.passType = pass;
	if (pass == MaterialPass::Transparent) {
		matData.pipeline = &transparentPipeline;
	}
	else {
		matData.pipeline = &opaquePipeline;
	}

	matData.materialSet = descriptorAllocator.allocate(device, materialLayout);


	writer.clear();
	writer.write_buffer(0, resources.dataBuffer, sizeof(MaterialConstants), resources.dataBufferOffset, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER);
	writer.write_image(1, resources.colorImage.imageView, resources.colorSampler, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER);
	writer.write_image(2, resources.metalRoughImage.imageView, resources.metalRoughSampler, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER);

	writer.update_set(device, matData.materialSet);

	return matData;
}

void MeshNode::Draw(const glm::mat4& topMatrix, DrawContext& ctx)
{
	m4 nodeMatrix = topMatrix * worldTransform;

	for (auto& s : mesh->surfaces) 
	{
		RenderObject def;
		def.indexCount = s.count;
		def.firstIndex = s.startIndex;
		def.indexBuffer = mesh->meshBuffers.indexBuffer.buffer;
		def.material = &s.material->data;
		def.bounds = s.bounds;
		def.transform = nodeMatrix;
		def.modelMat = topMatrix;
		def.vertexBufferAddress = mesh->meshBuffers.vertexBufferAddress;

		if (s.material->data.passType == MaterialPass::Transparent) {
			ctx.TransparentSurfaces.push_back(def);
		}
		else {
			ctx.OpaqueSurfaces.push_back(def);
		}
	}

	// recurse down
	Node::Draw(topMatrix, ctx);
}

void GameObject::Update()
{
	// Update Model Mat
	updateModelMat();

	// Update Hitboxes
	for (auto& hb : _vOBB) 
	{
		hb.update(_transform, _modelMat);
	}


}
