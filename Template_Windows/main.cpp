// TemplateWindows.cpp : Defines the entry point for the application.
//

#include "stdafx.h"
#include "main.h"
#include "wiGraphicsDevice_DX12.h"
#include <execution>
#include "wiEnums.h"

#define MAX_LOADSTRING 100

// Global Variables:
HINSTANCE hInst;                                // current instance
WCHAR szTitle[MAX_LOADSTRING];                  // The title bar text
WCHAR szWindowClass[MAX_LOADSTRING];            // the main window class name
// wi::Application application;					// Wicked Engine Application

// Forward declarations of functions included in this code module:
ATOM                MyRegisterClass(HINSTANCE hInstance);
BOOL                InitInstance(HINSTANCE, int);
LRESULT CALLBACK    WndProc(HWND, UINT, WPARAM, LPARAM);
INT_PTR CALLBACK    About(HWND, UINT, WPARAM, LPARAM);


std::unique_ptr<wi::graphics::GraphicsDevice_DX12> graphicsDevice;
using namespace wi::graphics;
using namespace wi::enums;


bool LoadShader(
	ShaderStage stage,
	Shader& shader,
	const std::string& filename
)
{
	std::string shaderbinaryfilename = wi::helper::GetCurrentPath() + "/../WickedEngine/shaders/" + filename;

	wi::shadercompiler::RegisterShader(shaderbinaryfilename);

	if (wi::shadercompiler::IsShaderOutdated(shaderbinaryfilename))
	{
		wi::shadercompiler::CompilerInput input;
		input.format = graphicsDevice->GetShaderFormat();
		input.stage = stage;
		input.minshadermodel = wi::graphics::ShaderModel::SM_6_0;
		input.defines = {};

		std::string sourcedir = wi::helper::GetCurrentPath() + "/../WickedEngine/shaders/";
		wi::helper::MakePathAbsolute(sourcedir);
		input.include_directories.push_back(sourcedir);
		input.include_directories.push_back(sourcedir + wi::helper::GetDirectoryFromPath(filename));
		input.shadersourcefilename = wi::helper::ReplaceExtension(sourcedir + filename, "hlsl");

		wi::shadercompiler::CompilerOutput output;
		wi::shadercompiler::Compile(input, output);

		if (output.IsValid())
		{
			wi::shadercompiler::SaveShaderAndMetadata(shaderbinaryfilename, output);

			if (!output.error_message.empty())
			{
				wi::backlog::post(output.error_message, wi::backlog::LogLevel::Warning);
			}
			wi::backlog::post("shader compiled: " + shaderbinaryfilename);
			return graphicsDevice->CreateShader(stage, output.shaderdata, output.shadersize, &shader);
		}
		else
		{
			wi::backlog::post("shader compile FAILED: " + shaderbinaryfilename + "\n" + output.error_message, wi::backlog::LogLevel::Error);
		}
	}

	if (graphicsDevice != nullptr)
	{
		wi::vector<uint8_t> buffer;
		if (wi::helper::FileRead(shaderbinaryfilename, buffer))
		{
			bool success = graphicsDevice->CreateShader(stage, buffer.data(), buffer.size(), &shader);
			return success;
		}
	}

	return false;
}

RasterizerState		rasterizers[RSTYPE_COUNT];
DepthStencilState	depthStencils[DSSTYPE_COUNT];
BlendState			blendStates[BSTYPE_COUNT];
GPUBuffer			buffers[BUFFERTYPE_COUNT];
Sampler				samplers[SAMPLER_COUNT];

void SetUpStates()
{
	RasterizerState rs;
	rs.fill_mode = FillMode::SOLID;
	rs.cull_mode = CullMode::BACK;
	rs.front_counter_clockwise = true;
	rs.depth_bias = 0;
	rs.depth_bias_clamp = 0;
	rs.slope_scaled_depth_bias = 0;
	rs.depth_clip_enable = true;
	rs.multisample_enable = false;
	rs.antialiased_line_enable = false;
	rs.conservative_rasterization_enable = false;
	rasterizers[RSTYPE_FRONT] = rs;

	DepthStencilState dsd;
	dsd.depth_enable = true;
	dsd.depth_write_mask = DepthWriteMask::ALL;
	dsd.depth_func = ComparisonFunc::GREATER;

	dsd.stencil_enable = true;
	dsd.stencil_read_mask = 0;
	dsd.stencil_write_mask = 0xFF;
	dsd.front_face.stencil_func = ComparisonFunc::ALWAYS;
	dsd.front_face.stencil_pass_op = StencilOp::REPLACE;
	dsd.front_face.stencil_fail_op = StencilOp::KEEP;
	dsd.front_face.stencil_depth_fail_op = StencilOp::KEEP;
	dsd.back_face.stencil_func = ComparisonFunc::ALWAYS;
	dsd.back_face.stencil_pass_op = StencilOp::REPLACE;
	dsd.back_face.stencil_fail_op = StencilOp::KEEP;
	dsd.back_face.stencil_depth_fail_op = StencilOp::KEEP;
	depthStencils[DSSTYPE_DEFAULT] = dsd;

	BlendState bd;
	bd.render_target[0].blend_enable = false;
	bd.render_target[0].render_target_write_mask = ColorWrite::ENABLE_ALL;
	bd.alpha_to_coverage_enable = false;
	bd.independent_blend_enable = false;
	blendStates[BSTYPE_OPAQUE] = bd;

	SamplerDesc samplerDesc;
	samplerDesc.filter = Filter::MIN_MAG_MIP_LINEAR;
	samplerDesc.address_u = TextureAddressMode::MIRROR;
	samplerDesc.address_v = TextureAddressMode::MIRROR;
	samplerDesc.address_w = TextureAddressMode::MIRROR;
	samplerDesc.mip_lod_bias = 0.0f;
	samplerDesc.max_anisotropy = 0;
	samplerDesc.comparison_func = ComparisonFunc::NEVER;
	samplerDesc.border_color = SamplerBorderColor::TRANSPARENT_BLACK;
	samplerDesc.min_lod = 0;
	samplerDesc.max_lod = std::numeric_limits<float>::max();
	graphicsDevice->CreateSampler(&samplerDesc, &samplers[SAMPLER_LINEAR_MIRROR]);

	samplerDesc.filter = Filter::MIN_MAG_MIP_LINEAR;
	samplerDesc.address_u = TextureAddressMode::CLAMP;
	samplerDesc.address_v = TextureAddressMode::CLAMP;
	samplerDesc.address_w = TextureAddressMode::CLAMP;
	graphicsDevice->CreateSampler(&samplerDesc, &samplers[SAMPLER_LINEAR_CLAMP]);
	graphicsDevice->CreateSampler(&samplerDesc, &samplers[SAMPLER_CMP_DEPTH]);
}

int APIENTRY wWinMain(_In_ HINSTANCE hInstance,
	_In_opt_ HINSTANCE hPrevInstance,
	_In_ LPWSTR    lpCmdLine,
	_In_ int       nCmdShow)
{
	UNREFERENCED_PARAMETER(hPrevInstance);
	UNREFERENCED_PARAMETER(lpCmdLine);

	// TODO: Place code here.

	BOOL dpi_success = SetProcessDpiAwarenessContext(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2);
	assert(dpi_success);

	wi::arguments::Parse(lpCmdLine); // if you wish to use command line arguments, here is a good place to parse them...

	// Initialize global strings
	LoadStringW(hInstance, IDS_APP_TITLE, szTitle, MAX_LOADSTRING);
	LoadStringW(hInstance, IDC_TEMPLATEWINDOWS, szWindowClass, MAX_LOADSTRING);
	MyRegisterClass(hInstance);

	// Perform application initialization:
	if (!InitInstance(hInstance, nCmdShow))
	{
		return FALSE;
	}

	HACCEL hAccelTable = LoadAccelerators(hInstance, MAKEINTRESOURCE(IDC_TEMPLATEWINDOWS));

	std::string ss;
	ss += "\n[wi::initializer] Initializing Wicked Engine, please wait...\n";
	ss += "Version: ";
	ss += wi::version::GetVersionString();
	wi::backlog::post(ss);
	wi::jobsystem::Initialize();
	graphicsDevice = std::make_unique<wi::graphics::GraphicsDevice_DX12>();

	SetUpStates();

	InputLayout layout;
	layout.elements.push_back(InputLayout::Element{ "POSITION", 0, Format::R32G32_FLOAT, 0, InputLayout::APPEND_ALIGNED_ELEMENT, InputClassification::PER_VERTEX_DATA });

	std::vector<size_t> stress(1000);
	std::for_each(std::execution::par_unseq, stress.begin(), stress.end(),
		[&](size_t i)
		{
			std::unique_ptr<Shader> vs = std::make_unique<Shader>();
			std::unique_ptr<Shader> ps = std::make_unique<Shader>();
			PipelineState pso;
			LoadShader(ShaderStage::VS, *vs, "testVS.cso");
			LoadShader(ShaderStage::PS, *ps, "testPS.cso");
		}
	);

	Shader vs;
	Shader ps;
	PipelineState pso;
	LoadShader(ShaderStage::VS, vs, "testVS.cso");
	LoadShader(ShaderStage::PS, ps, "testPS.cso");
	PipelineStateDesc psd{};
	psd.vs = &vs;
	psd.ps = &ps;
	psd.il = &layout;
	psd.dss = &depthStencils[DSSTYPE_DEFAULT];
	psd.rs = &rasterizers[RSTYPE_FRONT];
	psd.bs = &blendStates[BSTYPE_OPAQUE];
	psd.pt = PrimitiveTopology::TRIANGLELIST;
	graphicsDevice->CreatePipelineState(&psd, &pso);

	Texture renderTarget;
	GPUBuffer vb;
	constexpr XMFLOAT2 data[]{ {0.0f, 0.35f}, {0.35f, -0.35f}, {-0.35f, -0.35f} };
	GPUBufferDesc d;
	d.size = sizeof(data);
	d.stride = sizeof(XMFLOAT2);
	d.bind_flags = BindFlag::VERTEX_BUFFER | BindFlag::SHADER_RESOURCE;
	d.misc_flags = ResourceMiscFlag::BUFFER_RAW;
	graphicsDevice->CreateBuffer(&d, data, &vb);

	TextureDesc td{};
	td.width = 500;
	td.height = 500;
	td.depth = 1;
	td.array_size = 1;
	td.mip_levels = 1;
	td.format = Format::R8G8B8A8_UNORM;
	td.sample_count = 1;
	td.bind_flags = BindFlag::SHADER_RESOURCE | BindFlag::RENDER_TARGET;
	graphicsDevice->CreateTexture(&td, nullptr, &renderTarget);

	MSG msg = { 0 };
	while (msg.message != WM_QUIT)
	{
		if (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE)) {
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
		else {


			std::vector<size_t> indices(32);
			std::vector<CommandList> cmds(indices.size());

			for (size_t i = 0; i < indices.size(); ++i)
			{
				indices[i] = i;
				cmds[i] = graphicsDevice->BeginCommandList();
			}

			std::for_each(std::execution::par_unseq, indices.begin(), indices.end(),
				[&](size_t i)
				{
					auto cmd = cmds[i];

					Viewport vp[1];
					vp[0].width = static_cast<float>(500);
					vp[0].height = static_cast<float>(500);
					vp[0].min_depth = 0.0f;
					vp[0].max_depth = 1.0f;
					graphicsDevice->BindViewports(1, vp, cmd);

					RenderPassImage img_nocrash[] = { RenderPassImage::RenderTarget(
						&renderTarget,
						RenderPassImage::LoadOp::CLEAR,
						RenderPassImage::StoreOp::STORE,
						ResourceState::SHADER_RESOURCE,
						ResourceState::SHADER_RESOURCE)
					};
					RenderPassImage img_crash[] = { RenderPassImage::RenderTarget(
						&renderTarget,
						RenderPassImage::LoadOp::CLEAR,
						RenderPassImage::StoreOp::STORE,
						i == 0 ? ResourceState::SHADER_RESOURCE : ResourceState::RENDERTARGET,
						i == indices.size() - 1 ? ResourceState::SHADER_RESOURCE : ResourceState::RENDERTARGET)
					};
					graphicsDevice->RenderPassBegin(img_crash, 1, cmd);

					graphicsDevice->BindPipelineState(&pso, cmd);
					const GPUBuffer* vbs[1];
					vbs[0] = &vb;
					uint32_t strides[1];
					strides[0] = sizeof(XMFLOAT2);
					uint64_t offsets[1];
					offsets[0] = 0;
					graphicsDevice->BindVertexBuffers(vbs, 0, 1, strides, offsets, cmd);
					graphicsDevice->Draw(3, 0, cmd);
					graphicsDevice->RenderPassEnd(cmd);
				});

			graphicsDevice->SubmitCommandLists();

		}
	}

    return (int) msg.wParam;
}



//
//  FUNCTION: MyRegisterClass()
//
//  PURPOSE: Registers the window class.
//
ATOM MyRegisterClass(HINSTANCE hInstance)
{
    WNDCLASSEXW wcex;

    wcex.cbSize = sizeof(WNDCLASSEX);

    wcex.style          = CS_HREDRAW | CS_VREDRAW;
    wcex.lpfnWndProc    = WndProc;
    wcex.cbClsExtra     = 0;
    wcex.cbWndExtra     = 0;
    wcex.hInstance      = hInstance;
    wcex.hIcon          = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_TEMPLATEWINDOWS));
    wcex.hCursor        = LoadCursor(nullptr, IDC_ARROW);
    wcex.hbrBackground  = (HBRUSH)(COLOR_WINDOW+1);
    wcex.lpszMenuName   = MAKEINTRESOURCEW(IDC_TEMPLATEWINDOWS);
    wcex.lpszClassName  = szWindowClass;
    wcex.hIconSm        = LoadIcon(wcex.hInstance, MAKEINTRESOURCE(IDI_SMALL));

    return RegisterClassExW(&wcex);
}

//
//   FUNCTION: InitInstance(HINSTANCE, int)
//
//   PURPOSE: Saves instance handle and creates main window
//
//   COMMENTS:
//
//        In this function, we save the instance handle in a global variable and
//        create and display the main program window.
//
BOOL InitInstance(HINSTANCE hInstance, int nCmdShow)
{
   hInst = hInstance; // Store instance handle in our global variable

   HWND hWnd = CreateWindowW(szWindowClass, szTitle, WS_OVERLAPPEDWINDOW,
      CW_USEDEFAULT, 0, CW_USEDEFAULT, 0, nullptr, nullptr, hInstance, nullptr);

   if (!hWnd)
   {
      return FALSE;
   }

   ShowWindow(hWnd, nCmdShow);
   UpdateWindow(hWnd);


   // application.SetWindow(hWnd); // assign window handle (mandatory)


   return TRUE;
}

//
//  FUNCTION: WndProc(HWND, UINT, WPARAM, LPARAM)
//
//  PURPOSE:  Processes messages for the main window.
//
//  WM_COMMAND  - process the application menu
//  WM_PAINT    - Paint the main window
//  WM_DESTROY  - post a quit message and return
//
//
LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    switch (message)
    {
    case WM_COMMAND:
        {
            int wmId = LOWORD(wParam);
            // Parse the menu selections:
            switch (wmId)
            {
            case IDM_ABOUT:
                DialogBox(hInst, MAKEINTRESOURCE(IDD_ABOUTBOX), hWnd, About);
                break;
            case IDM_EXIT:
                DestroyWindow(hWnd);
                break;
            default:
                return DefWindowProc(hWnd, message, wParam, lParam);
            }
        }
        break;
    case WM_SIZE:
    case WM_DPICHANGED:
		//if (application.is_window_active)
		//	application.SetWindow(hWnd);
        break;
	case WM_CHAR:
		switch (wParam)
		{
		case VK_BACK:
			wi::gui::TextInputField::DeleteFromInput();
			break;
		case VK_RETURN:
			break;
		default:
		{
			const wchar_t c = (const wchar_t)wParam;
			wi::gui::TextInputField::AddInput(c);
		}
		break;
		}
		break;
	case WM_INPUT:
		wi::input::rawinput::ParseMessage((void*)lParam);
		break;
	case WM_KILLFOCUS:
		break;
	case WM_SETFOCUS:
		break;
    case WM_PAINT:
        {
            PAINTSTRUCT ps;
            HDC hdc = BeginPaint(hWnd, &ps);
            // TODO: Add any drawing code that uses hdc here...
            EndPaint(hWnd, &ps);
        }
        break;
    case WM_DESTROY:
        PostQuitMessage(0);
        break;
    default:
        return DefWindowProc(hWnd, message, wParam, lParam);
    }
    return 0;
}

// Message handler for about box.
INT_PTR CALLBACK About(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
    UNREFERENCED_PARAMETER(lParam);
    switch (message)
    {
    case WM_INITDIALOG:
        return (INT_PTR)TRUE;

    case WM_COMMAND:
        if (LOWORD(wParam) == IDOK || LOWORD(wParam) == IDCANCEL)
        {
            EndDialog(hDlg, LOWORD(wParam));
            return (INT_PTR)TRUE;
        }
        break;
    }
    return (INT_PTR)FALSE;
}
