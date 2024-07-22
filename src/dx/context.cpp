#include "context.h"

static void enable_debug_layer()
{
#if defined(_DEBUG)
	com<ID3D12Debug3> debugInterface;
	check_dx(D3D12GetDebugInterface(IID_PPV_ARGS(&debugInterface)));
	debugInterface->EnableDebugLayer();
#endif
}

static DXFactory create_factory()
{
	u32 factory_flags = 0;
#if defined(_DEBUG)
	factory_flags = DXGI_CREATE_FACTORY_DEBUG;
#endif

	DXFactory factory;
	check_dx(CreateDXGIFactory2(factory_flags, IID_PPV_ARGS(&factory)));

	return factory;
}

static std::pair<DXAdapter, D3D_FEATURE_LEVEL> get_adapter(DXFactory factory, D3D_FEATURE_LEVEL min_feature_level = D3D_FEATURE_LEVEL_12_0)
{
	com<IDXGIAdapter1> adapter_1;
	DXAdapter adapter;

	D3D_FEATURE_LEVEL feature_level = D3D_FEATURE_LEVEL_9_1;

	D3D_FEATURE_LEVEL possible_feature_levels[] =
	{
		D3D_FEATURE_LEVEL_9_1,
		D3D_FEATURE_LEVEL_9_2,
		D3D_FEATURE_LEVEL_9_3,
		D3D_FEATURE_LEVEL_10_0,
		D3D_FEATURE_LEVEL_10_1,
		D3D_FEATURE_LEVEL_11_0,
		D3D_FEATURE_LEVEL_11_1,
		D3D_FEATURE_LEVEL_12_0,
		D3D_FEATURE_LEVEL_12_1,
		D3D_FEATURE_LEVEL_12_2,
	};

	u32 first_feature_level = 0;
	for (u32 i = 0; i < arraysize(possible_feature_levels); ++i)
	{
		if (possible_feature_levels[i] == min_feature_level)
		{
			first_feature_level = i;
			break;
		}
	}

	u64 max_dedicated_device_memory = 0;
	for (u32 i = 0; factory->EnumAdapters1(i, &adapter_1) != DXGI_ERROR_NOT_FOUND; ++i)
	{
		DXGI_ADAPTER_DESC1 adapter_desc;
		check_dx(adapter_1->GetDesc1(&adapter_desc));

		// Check to see if the adapter can create a D3D12 device without actually 
		// creating it. Out of all adapters which support the minimum feature level,
		// the adapter with the largest dedicated video memory is favored.
		if ((adapter_desc.Flags & DXGI_ADAPTER_FLAG_SOFTWARE) == 0)
		{
			D3D_FEATURE_LEVEL adapter_feature_level = D3D_FEATURE_LEVEL_9_1;
			bool supports_feature_level = false;

			for (u32 fl = first_feature_level; fl < arraysize(possible_feature_levels); ++fl)
			{
				if (SUCCEEDED(D3D12CreateDevice(adapter_1.Get(),
					possible_feature_levels[fl], __uuidof(ID3D12Device), 0)))
				{
					adapter_feature_level = possible_feature_levels[fl];
					supports_feature_level = true;
				}
			}

			if (supports_feature_level && adapter_desc.DedicatedVideoMemory > max_dedicated_device_memory)
			{
				check_dx(adapter_1.As(&adapter));
				max_dedicated_device_memory = adapter_desc.DedicatedVideoMemory;
				feature_level = adapter_feature_level;
			}
		}
	}

	return { adapter, feature_level };
}

static std::pair<DXAdapter, DXDevice> create_device(DXFactory factory, D3D_FEATURE_LEVEL min_feature_level = D3D_FEATURE_LEVEL_12_0)
{
	auto [adapter, feature_level] = get_adapter(factory, min_feature_level);

	DXDevice device;
	check_dx(D3D12CreateDevice(adapter.Get(), feature_level, IID_PPV_ARGS(&device)));

#if defined(_DEBUG)
	com<ID3D12InfoQueue> info_queue;
	if (SUCCEEDED(device.As(&info_queue)))
	{
		info_queue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_CORRUPTION, TRUE);
		info_queue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_ERROR, TRUE);
		info_queue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_WARNING, TRUE);

		// Suppress whole categories of messages.
		//D3D12_MESSAGE_CATEGORY categories[] = {};

		// Suppress messages based on their severity level.
		D3D12_MESSAGE_SEVERITY severities[] =
		{
			D3D12_MESSAGE_SEVERITY_INFO
		};

		// Suppress individual messages by their ID.
		D3D12_MESSAGE_ID ids[] = {
			D3D12_MESSAGE_ID_CLEARRENDERTARGETVIEW_MISMATCHINGCLEARVALUE,   // I'm really not sure how to avoid this message.
			D3D12_MESSAGE_ID_MAP_INVALID_NULLRANGE,                         // This warning occurs when using capture frame while graphics debugging.
			D3D12_MESSAGE_ID_UNMAP_INVALID_NULLRANGE,                       // This warning occurs when using capture frame while graphics debugging.
			D3D12_MESSAGE_ID_OBJECT_ACCESSED_WHILE_STILL_IN_USE,			// Disabled to allow OBS to capture the window.
		};

		D3D12_INFO_QUEUE_FILTER filter = {};
		//filter.DenyList.NumCategories = arraysize(categories);
		//filter.DenyList.pCategoryList = categories;
		filter.DenyList.NumSeverities = arraysize(severities);
		filter.DenyList.pSeverityList = severities;
		filter.DenyList.NumIDs = arraysize(ids);
		filter.DenyList.pIDList = ids;

		check_dx(info_queue->PushStorageFilter(&filter));
	}
#endif

	return { adapter, device };
}

static void create_queue(DXCommandQueue& queue, DXDevice device, D3D12_COMMAND_LIST_TYPE type)
{
	queue.command_list_type = type;

	D3D12_COMMAND_QUEUE_DESC desc = {};
	desc.Type = type;
	desc.Priority = D3D12_COMMAND_QUEUE_PRIORITY_NORMAL;
	desc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
	desc.NodeMask = 0;

	check_dx(device->CreateCommandQueue(&desc, IID_PPV_ARGS(&queue.command_queue)));
	check_dx(device->CreateFence(queue.next_fence_value, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&queue.fence)));

	switch (type)
	{
		case D3D12_COMMAND_LIST_TYPE_DIRECT: queue.command_queue->SetName(L"Render command queue"); break;
		case D3D12_COMMAND_LIST_TYPE_COMPUTE: queue.command_queue->SetName(L"Compute command queue"); break;
		case D3D12_COMMAND_LIST_TYPE_COPY: queue.command_queue->SetName(L"Copy command queue"); break;
	}
}

static std::string get_last_error_as_string()
{
	//Get the error message ID, if any.
	DWORD errorMessageID = GetLastError();
	if (errorMessageID == 0) 
	{
		return std::string(); //No error message has been recorded
	}

	LPSTR messageBuffer = nullptr;

	//Ask Win32 to give us the string version of that message ID.
	//The parameters we pass in, tell Win32 to create the buffer that holds the message for us (because we don't yet know how long the message string will be).
	size_t size = FormatMessageA(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
		NULL, errorMessageID, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), (LPSTR)&messageBuffer, 0, NULL);

	//Copy the error message into a std::string.
	std::string message(messageBuffer, size);

	//Free the Win32's string's buffer.
	LocalFree(messageBuffer);

	return message;
}

static std::thread create_command_list_cleanup_thread(DXCommandQueue& render_queue, DXCommandQueue& compute_queue, DXCommandQueue& copy_queue, HANDLE end_event)
{
	return std::thread([&render_queue, &compute_queue, &copy_queue, end_event]()
	{
		DXCommandQueue* queues[3] = { &render_queue, &compute_queue, &copy_queue };

		while (true)
		{
			i32 valid_count = 0;
			HANDLE waiting_events[4] = {};
			DXCommandQueue* waiting_queues[3] = {};

			for (i32 i = 0; i < 3; ++i)
			{
				DXCommandQueue* queue = queues[i];
				DXCommandList* cl = queue->peek_oldest_running_command_list();
				if (cl)
				{
					waiting_events[valid_count] = CreateEvent(NULL, FALSE, FALSE, NULL);
					waiting_queues[valid_count] = queues[i];
					queues[i]->fence->SetEventOnCompletion(cl->last_fence_value, waiting_events[valid_count]);
					++valid_count;
				}
			}

			waiting_events[valid_count++] = end_event;

			DWORD wait_result = WaitForMultipleObjects(valid_count, waiting_events, false, 4);
			if (wait_result == WAIT_FAILED)
			{
				std::cerr << get_last_error_as_string() << '\n';
				continue;
			}
			if (wait_result == WAIT_TIMEOUT)
			{
				continue;
			}

			if (wait_result >= WAIT_OBJECT_0 && wait_result < WAIT_OBJECT_0 + valid_count)
			{
				i32 event_index = wait_result - WAIT_OBJECT_0;
				if (event_index == valid_count - 1)
				{
					// End event triggered
					break;
				}

				DXCommandQueue* queue = waiting_queues[event_index];
				DXCommandList* cl = queue->pop_oldest_running_command_list();
				ASSERT(cl);
				cl->reset();

				queue->add_free_command_list(cl);

				CloseHandle(waiting_events[event_index]);
			}
		}
	});
}

DXContext::DXContext()
{
	SetThreadDpiAwarenessContext(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2);
	check_dx(CoInitializeEx(NULL, COINIT_APARTMENTTHREADED | COINIT_DISABLE_OLE1DDE));


	enable_debug_layer();
	
	factory = create_factory();
	std::tie(adapter, device) = create_device(factory);

	create_queue(render_queue, device, D3D12_COMMAND_LIST_TYPE_DIRECT);
	create_queue(compute_queue, device, D3D12_COMMAND_LIST_TYPE_COMPUTE);
	create_queue(copy_queue, device, D3D12_COMMAND_LIST_TYPE_COPY);

	end_event = CreateEvent(NULL, FALSE, FALSE, NULL);
	command_list_cleanup_thread = create_command_list_cleanup_thread(render_queue, compute_queue, copy_queue, end_event);

	for (u32 i = 0; i < NUM_BUFFERED_FRAMES; ++i)
	{
		render_graveyard[i].queue = &render_queue;
	}
	copy_graveyard.queue = &copy_queue;

	for (u32 i = 0; i < NUM_BUFFERED_FRAMES; ++i)
	{
		frame_scratch[i].initialize(MB(8));
	}
}

DXContext::~DXContext()
{
	SetEvent(end_event);
	command_list_cleanup_thread.join();
}

float DXContext::begin_frame()
{
	++frame_id;

	render_graveyard[wrapping_frame_id()].cleanup();
	copy_graveyard.cleanup();
	frame_scratch[wrapping_frame_id()].reset();


	std::chrono::time_point<std::chrono::high_resolution_clock> now = std::chrono::high_resolution_clock::now();
	std::chrono::duration<double> duration = now - last_timepoint;
	float delta_time = (float)duration.count();
	last_timepoint = now;
	return delta_time;
}

void DXContext::flush()
{
	render_queue.flush();
	compute_queue.flush();
	copy_queue.flush();
}

DXCommandList* DXContext::get_free_render_command_list()
{
	DXCommandList* cl = render_queue.get_free_command_list();
	CD3DX12_RECT scissor_rect(0, 0, LONG_MAX, LONG_MAX);
	cl->set_scissor(scissor_rect);
	return cl;
}

DXCommandList* DXContext::get_free_compute_command_list()
{
	return compute_queue.get_free_command_list();
}

DXCommandList* DXContext::get_free_copy_command_list()
{
	return copy_queue.get_free_command_list();
}

u64 DXContext::execute_command_list(DXCommandList* cl)
{
	switch (cl->type)
	{
		case D3D12_COMMAND_LIST_TYPE_DIRECT: return render_queue.execute_command_list(cl);
		case D3D12_COMMAND_LIST_TYPE_COMPUTE: return compute_queue.execute_command_list(cl);
		case D3D12_COMMAND_LIST_TYPE_COPY: return copy_queue.execute_command_list(cl);
	}
	return 0;
}

u32 DXContext::get_free_command_lists_count()
{
	return render_queue.get_free_command_lists_count() + compute_queue.get_free_command_lists_count() + copy_queue.get_free_command_lists_count();
}

u32 DXContext::get_running_command_lists_count()
{
	return render_queue.get_running_command_lists_count() + compute_queue.get_running_command_lists_count() + copy_queue.get_running_command_lists_count();
}

void DXContext::keep_render_resource_alive(DXResource resource)
{
	render_graveyard[wrapping_frame_id()].add_resource(0, resource);
}

void DXContext::keep_copy_resource_alive(u64 fence_value, DXResource resource)
{
	copy_graveyard.add_resource(fence_value, resource);
}

DXAllocation DXContext::allocate_frame_scratch(u64 size, u64 alignment)
{
	return frame_scratch[wrapping_frame_id()].allocate(size, alignment);
}

DXContext dx_context;
