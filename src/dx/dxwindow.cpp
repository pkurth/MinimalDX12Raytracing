#include "dxwindow.h"
#include "context.h"

static bool check_tearing_support(DXFactory factory)
{
	BOOL support = false;

	// Rather than create the DXGI 1.5 factory interface directly, we create the
	// DXGI 1.4 interface and query for the 1.5 interface. This is to enable the 
	// graphics debugging tools which will not support the 1.5 factory interface 
	// until a future update.
	com<IDXGIFactory5> factory5;
	if (SUCCEEDED(factory.As(&factory5)))
	{
		if (FAILED(factory5->CheckFeatureSupport(
			DXGI_FEATURE_PRESENT_ALLOW_TEARING,
			&support, sizeof(support))))
		{
			support = false;
		}
	}

	return support;
}

static DXSwapchain create_swapchain(HWND window_handle, DXFactory factory, const DXCommandQueue& queue, u32 width, u32 height, u32 buffer_count, bool tearing_supported)
{
	DXSwapchain swapchain;

	DXGI_SWAP_CHAIN_DESC1 swapchain_desc = {};
	swapchain_desc.Width = width;
	swapchain_desc.Height = height;
	swapchain_desc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	swapchain_desc.Stereo = FALSE;
	swapchain_desc.SampleDesc = { 1, 0 };
	swapchain_desc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
	swapchain_desc.BufferCount = buffer_count;
	swapchain_desc.Scaling = DXGI_SCALING_STRETCH;
	swapchain_desc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
	swapchain_desc.AlphaMode = DXGI_ALPHA_MODE_UNSPECIFIED;

	// It is recommended to always allow tearing if tearing support is available.
	swapchain_desc.Flags = tearing_supported ? DXGI_SWAP_CHAIN_FLAG_ALLOW_TEARING : 0;

	com<IDXGISwapChain1> swapchain1;
	check_dx(factory->CreateSwapChainForHwnd(
		queue.command_queue.Get(),
		window_handle,
		&swapchain_desc,
		0,
		0,
		&swapchain1));

	UINT flags = 0;

	// Disable the Alt+Enter fullscreen toggle feature. Switching to fullscreen
	// will be handled manually.
	flags |= DXGI_MWA_NO_ALT_ENTER;

	check_dx(factory->MakeWindowAssociation(window_handle, flags));

	check_dx(swapchain1.As(&swapchain));

	return swapchain;
}

DXWindow::DXWindow(const TCHAR* name, u32 client_width, u32 client_height)
	: Window(name, client_width, client_height)
{
	if (!Window::initialized())
	{
		return;
	}

	tearing_supported = check_tearing_support(dx_context.factory);


	swapchain = create_swapchain(window_handle, dx_context.factory, dx_context.render_queue, client_width, client_height, NUM_BUFFERED_FRAMES, tearing_supported);
	current_backbuffer_index = swapchain->GetCurrentBackBufferIndex();

	D3D12_DESCRIPTOR_HEAP_DESC descriptor_heap_desc = {};
	descriptor_heap_desc.NumDescriptors = NUM_BUFFERED_FRAMES;
	descriptor_heap_desc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;

	check_dx(dx_context.device->CreateDescriptorHeap(&descriptor_heap_desc, IID_PPV_ARGS(&rtv_descriptor_heap)));

	update_rtvs();
}

DXWindow::~DXWindow()
{
	dx_context.flush();
}

bool DXWindow::begin_frame()
{
	if (Window::begin_frame())
	{
		u32 index = current_backbuffer_index;
		dx_context.render_queue.wait_for_fence(fence_values[index]);
		return true;
	}

	return {};
}

void DXWindow::end_frame(u64 fence)
{
	if (initialized())
	{
		u32 sync_interval = vsync ? 1 : 0;
		u32 present_flags = tearing_supported && !vsync ? DXGI_PRESENT_ALLOW_TEARING : 0;
		check_dx(swapchain->Present(sync_interval, present_flags));

		current_backbuffer_index = swapchain->GetCurrentBackBufferIndex();

		fence_values[current_backbuffer_index] = fence;
	}
}

u64 DXWindow::blit_to_screen(const DXResource& image)
{
	DXResource backbuffer = backbuffers[current_backbuffer_index];

	DXCommandList* cl = dx_context.get_free_render_command_list();
	//cl->transition(backbuffer, D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_COPY_DEST);
	cl->copy_resource(image, backbuffer);
	cl->transition(backbuffer, D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_PRESENT);
	u64 fence = dx_context.execute_command_list(cl);
	return fence;
}

u64 DXWindow::blit_to_screen(std::shared_ptr<DXTexture> image)
{
	return blit_to_screen(image->resource);
}

CD3DX12_VIEWPORT DXWindow::get_viewport()
{
	return CD3DX12_VIEWPORT(0.f, 0.f, (float)client_width, (float)client_height);
}

void DXWindow::on_resize()
{
	dx_context.flush();

	for (u32 i = 0; i < NUM_BUFFERED_FRAMES; ++i)
	{
		backbuffers[i].Reset();
	}

	DXGI_SWAP_CHAIN_DESC swapchain_desc = {};
	check_dx(swapchain->GetDesc(&swapchain_desc));
	check_dx(swapchain->ResizeBuffers(NUM_BUFFERED_FRAMES, client_width, client_height, swapchain_desc.BufferDesc.Format, swapchain_desc.Flags));

	current_backbuffer_index = swapchain->GetCurrentBackBufferIndex();

	update_rtvs();
}

void DXWindow::update_rtvs()
{
	u32 rtv_descriptor_size = dx_context.device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);

	CD3DX12_CPU_DESCRIPTOR_HANDLE rtvHandle(rtv_descriptor_heap->GetCPUDescriptorHandleForHeapStart());

	for (u32 i = 0; i < NUM_BUFFERED_FRAMES; ++i)
	{
		DXResource backbuffer;
		check_dx(swapchain->GetBuffer(i, IID_PPV_ARGS(&backbuffer)));

		dx_context.device->CreateRenderTargetView(backbuffer.Get(), 0, rtvHandle);

		backbuffer->SetName(L"Backbuffer");

		backbuffers[i] = backbuffer;
		backbuffer_rtvs[i] = rtvHandle;

		rtvHandle.Offset(rtv_descriptor_size);
	}
}
