#if defined (WIN32)
#include <atlbase.h> 
#include <dxc/dxcapi.h>
#include <d3d12shader.h>
#endif

#include "ShaderCompiler.h"
#include "utils/StringUtils.h"
#include <atomic>
#include <filesystem>

#include "utils/DataStructures.h"

namespace Device
{
	namespace ShaderCompiler
	{
		const std::map<ShaderProgram::Stage, std::wstring> stage_to_target = {
			{ ShaderProgram::Stage::Vertex, L"vs_6_0" },
			{ ShaderProgram::Stage::Fragment, L"ps_6_0" },
			{ ShaderProgram::Stage::Compute, L"cs_6_0" },
		};

		template<typename TObject>
		HRESULT DoBasicQueryInterface_recurse(TObject* self, REFIID iid, void** ppvObject) {
			return E_NOINTERFACE;
		}
		template<typename TObject, typename TInterface, typename... Ts>
		HRESULT DoBasicQueryInterface_recurse(TObject* self, REFIID iid, void** ppvObject) {
			if (ppvObject == nullptr) return E_POINTER;
			if (IsEqualIID(iid, __uuidof(TInterface))) {
				*(TInterface**)ppvObject = self;
				self->AddRef();
				return S_OK;
			}
			return DoBasicQueryInterface_recurse<TObject, Ts...>(self, iid, ppvObject);
		}
		template<typename... Ts, typename TObject>
		HRESULT DoBasicQueryInterface(TObject* self, REFIID iid, void** ppvObject) {
			if (ppvObject == nullptr) return E_POINTER;

			// Support INoMarshal to void GIT shenanigans.
			if (IsEqualIID(iid, __uuidof(IUnknown)) ||
				IsEqualIID(iid, __uuidof(INoMarshal))) {
				*ppvObject = reinterpret_cast<IUnknown*>(self);
				reinterpret_cast<IUnknown*>(self)->AddRef();
				return S_OK;
			}

			return DoBasicQueryInterface_recurse<TObject, Ts...>(self, iid, ppvObject);
		}
	 

		bool CompileShader(ShaderCache& shader_cache, const ShaderProgramInfo::ShaderData& shader_data, CompilationResult& out_result)
		{
#if defined (WIN32)
			struct Blob : IDxcBlob
			{
				const std::vector<uint8_t>& data;

				Blob(const std::vector<uint8_t>& data) : data(data){}

				virtual LPVOID STDMETHODCALLTYPE GetBufferPointer(void)
				{
					return (LPVOID)data.data();
				}

				virtual SIZE_T STDMETHODCALLTYPE GetBufferSize(void)
				{
					return data.size();
				}

				unsigned long AddRef() override {
					return 1;
				}
				unsigned long Release() override {
					return 1;
				}

				HRESULT STDMETHODCALLTYPE QueryInterface(REFIID iid, void** ppvObject) override
				{
					return DoBasicQueryInterface<IDxcBlob>(this, iid, ppvObject);
				}
			};



			struct IncludeHandler : public IDxcIncludeHandler
			{
				unsigned long AddRef() override {
					return 1;
				}
				unsigned long Release() override {
					return 1;
				}

				HRESULT STDMETHODCALLTYPE QueryInterface(REFIID iid, void** ppvObject) override
				{
					return DoBasicQueryInterface<IDxcIncludeHandler>(this, iid, ppvObject);
				}

				ShaderCache& shader_cache;

				IncludeHandler(ShaderCache& shader_cache) : shader_cache(shader_cache) {}

				virtual HRESULT STDMETHODCALLTYPE LoadSource(_In_z_ LPCWSTR pFilename, _COM_Outptr_result_maybenull_ IDxcBlob** ppIncludeSource) override
				{
					std::wstring filename(pFilename);

					auto it = blobs.find(filename);
					if (it != blobs.end())
					{
						*ppIncludeSource = it->second.get();
					}
					else
					{
						auto& data = shader_cache.GetShaderSource(filename);
						if (data.size())
						{
							auto blob = std::make_unique<Blob>(data);
							*ppIncludeSource = blob.get();
							blobs.insert(std::make_pair(filename, std::move(blob)));
						}
						else
						{
							*ppIncludeSource = nullptr;
							return E_FAIL;
						}
					}

					return S_OK;
				}

				std::unordered_map<std::wstring, std::unique_ptr<Blob>> blobs;
			};

			OPTICK_EVENT();
			auto& shader_source = shader_cache.GetShaderSource(shader_data.path);
			if (!shader_source.size())
			{
				out_result.error = "Can't open shader source file " + utils::WStringToString(shader_data.path);
				return false;
			}

			IncludeHandler include_handler(shader_cache);

			CComPtr<IDxcUtils> pUtils;
			CComPtr<IDxcCompiler3> pCompiler;
			DxcCreateInstance(CLSID_DxcUtils, IID_PPV_ARGS(&pUtils));
			DxcCreateInstance(CLSID_DxcCompiler, IID_PPV_ARGS(&pCompiler));

			auto entry_point = utils::StringToWString(shader_data.entry_point.c_str());

			std::vector<std::wstring> defines;
			std::wstringstream stream;
			for (auto& define : shader_data.defines)
			{
				stream.clear();
				stream.str(L"");
				stream << utils::StringToWString(define.name) << L"=" << utils::StringToWString(define.value);
				defines.push_back(stream.str());
			}

			utils::SmallVector<LPCWSTR, 20> args;
			args.push_back(shader_data.path.c_str());
			args.push_back(L"-E"); args.push_back(entry_point.c_str());
			args.push_back(L"-T"); args.push_back(stage_to_target.at(shader_data.stage).c_str());
			args.push_back(L"-Zi");
			args.push_back(L"-spirv"); args.push_back(L"-fvk-use-gl-layout");
			for (auto& define : defines)
			{
				args.push_back(L"-D"); args.push_back(define.c_str());
			}

			DxcBuffer Source;
			Source.Ptr = shader_source.data();
			Source.Size = shader_source.size();
			Source.Encoding = DXC_CP_ACP; // Assume BOM says UTF8 or UTF16 or this is ANSI text.

			CComPtr<IDxcResult> pResults;
			pCompiler->Compile(
				&Source,
				args.data(),
				args.size(),
				&include_handler,
				IID_PPV_ARGS(&pResults)
			);

			CComPtr<IDxcBlobUtf8> pErrors = nullptr;
			pResults->GetOutput(DXC_OUT_ERRORS, IID_PPV_ARGS(&pErrors), nullptr);
			// Note that d3dcompiler would return null if no errors or warnings are present.  
			// IDxcCompiler3::Compile will always return an error buffer, but its length will be zero if there are no warnings or errors.
			if (pErrors != nullptr && pErrors->GetStringLength() != 0)
			{
				out_result.error = pErrors->GetStringPointer();
				wprintf(L"Warnings and Errors:\n%S\n", pErrors->GetStringPointer());
			}

			HRESULT hrStatus;
			pResults->GetStatus(&hrStatus);
			if (FAILED(hrStatus))
			{
				wprintf(L"Compilation Failed\n");
				return false;
			}

			CComPtr<IDxcBlob> pShader = nullptr;
			CComPtr<IDxcBlobUtf16> pShaderName = nullptr;
			pResults->GetOutput(DXC_OUT_OBJECT, IID_PPV_ARGS(&pShader), &pShaderName);
			if (pShader != nullptr)
			{
				out_result.data.resize(pShader->GetBufferSize());
				memcpy_s(out_result.data.data(), out_result.data.size(), pShader->GetBufferPointer(), pShader->GetBufferSize());
			}
			else
				return false;

			return true;
#else
			return false;
#endif
		}
	}
}