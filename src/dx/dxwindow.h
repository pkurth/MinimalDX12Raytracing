#pragma once

#include "core/window.h"
#include "dx.h"


struct DXWindow : Window
{
	DXWindow(const TCHAR* name, u32 client_width, u32 client_height);
	~DXWindow();

	void set_vsync(bool vsync) { this->vsync = vsync; }

	bool begin_frame();
	void end_frame(u64 fence);

	u64 blit_to_screen(const DXResource& image);

	CD3DX12_VIEWPORT get_viewport();

	void on_resize() override;

private:

	bool tearing_supported;
	DXSwapchain swapchain;
	com<ID3D12DescriptorHeap> rtv_descriptor_heap;

	DXResource backbuffers[NUM_BUFFERED_FRAMES];
	CD3DX12_CPU_DESCRIPTOR_HANDLE backbuffer_rtvs[NUM_BUFFERED_FRAMES];
	u32 current_backbuffer_index = 0;
	bool vsync = false;

	u64 fence_values[NUM_BUFFERED_FRAMES] = {};

	void update_rtvs();
};
