#pragma once
#include <d3d11.h>
#include <Atlbase.h>
#include <Windows.h>
#include <dxgi.h>
#include <DirectXMath.h>
#include <DirectXMathVector.inl>
#include <string>
#include <vector>
#include <memory>
#include <array>
#include <map>
#include "../../frame/define.h"
namespace PO
{
	namespace Dx11
	{
		using float2 = DirectX::XMFLOAT2;
		using float3 = DirectX::XMFLOAT3;
		using float4 = DirectX::XMFLOAT4;
		using matrix4 = DirectX::XMMATRIX;

		template<typename store> struct store2
		{
			store x;
			store y;
		};

		template<typename store> struct store3
		{
			store x;
			store y;
			store z;
		};
		template<typename store> struct store4
		{
			store x;
			store y;
			store z;
			store w;
		};

		using int2 = store2<int32_t>;
		using int3 = store3<int32_t>;
		using int4 = store4<int32_t>;

		using uint2 = store2<uint32_t>;
		using uint3 = store3<uint32_t>;
		using uint4 = store4<uint32_t>;
	}

	namespace DXGI
	{
		template<typename T> struct data_format;
		template<> struct data_format<Dx11::float2>
		{
			static constexpr DXGI_FORMAT format = DXGI_FORMAT::DXGI_FORMAT_R32G32_FLOAT;
		};
		template<> struct data_format<Dx11::float3>
		{
			static constexpr DXGI_FORMAT format = DXGI_FORMAT::DXGI_FORMAT_R32G32B32_FLOAT;
		};
		template<> struct data_format<Dx11::float4>
		{
			static constexpr DXGI_FORMAT format = DXGI_FORMAT::DXGI_FORMAT_R32G32B32A32_FLOAT;
		};
		template<> struct data_format<Dx11::int2>
		{
			static constexpr DXGI_FORMAT format = DXGI_FORMAT::DXGI_FORMAT_R32G32_SINT;
		};
		template<> struct data_format<Dx11::int3>
		{
			static constexpr DXGI_FORMAT format = DXGI_FORMAT::DXGI_FORMAT_R32G32B32_SINT;
		};
	}

	namespace Dx11
	{
		namespace Implement
		{
			using resource_ptr = CComPtr<ID3D11Device>;
			using context_ptr = CComPtr<ID3D11DeviceContext>;
			using chain_ptr = CComPtr<IDXGISwapChain>;
			using buffer_ptr = CComPtr<ID3D11Buffer>;
			using vshader_ptr = CComPtr<ID3D11VertexShader>;
			using pshader_ptr = CComPtr<ID3D11PixelShader>;
			using layout_ptr = CComPtr<ID3D11InputLayout>;
			using raterizer_state_ptr = CComPtr<ID3D11RasterizerState>;
			using texture2D_ptr = CComPtr<ID3D11Texture2D>;
			using texture1D_ptr = CComPtr<ID3D11Texture1D>;
			using texture3D_ptr = CComPtr<ID3D11Texture3D>;
			using resource_view_ptr = CComPtr<ID3D11ShaderResourceView>;
			using sample_state_ptr = CComPtr<ID3D11SamplerState>;
			using cshader_ptr = CComPtr<ID3D11ComputeShader>;
			using gshader_ptr = CComPtr<ID3D11GeometryShader>;
		}

		namespace Purpose
		{
			struct buffer_purpose
			{
				D3D11_USAGE usage;
				UINT cpu_flag;
				UINT additional_bind;
			};
			extern buffer_purpose input;
			extern buffer_purpose output;
			extern buffer_purpose constant;
			extern buffer_purpose transfer;
		}

		struct buffer
		{
			Implement::buffer_ptr ptr;
			//uint64_t vision;
			size_t buffer_size;
			buffer& operator= (const buffer& b)
			{
				ptr = b.ptr;
				buffer_size = b.buffer_size;
				return *this;
			}
			bool create(Implement::resource_ptr& rp, D3D11_USAGE usage, UINT cpu_flag, UINT bind_flag, const void* data, size_t data_size, UINT misc_flag, size_t StructureByteStride);
			template<typename T> auto write(Implement::context_ptr& cp, T& t) -> Tool::optional<decltype(t(static_cast<void*>(nullptr), static_cast<size_t>(0)))>
			{
				if (ptr != nullptr)
				{
					D3D11_BUFFER_DESC DBD;
					ptr->GetDesc(&DBD);
					if (
						(DBD.Usage == D3D11_USAGE::D3D11_USAGE_DYNAMIC || DBD.Usage == D3D11_USAGE::D3D11_USAGE_STAGING)
						&& (DBD.CPUAccessFlags & D3D11_CPU_ACCESS_FLAG::D3D11_CPU_ACCESS_WRITE == D3D11_CPU_ACCESS_FLAG::D3D11_CPU_ACCESS_WRITE)
						)
					{
						D3D11_MAPPED_SUBRESOURCE DMS;
						if (cp->Map(ptr, 0, D3D11_MAP_WRITE, &DMS) != S_OK)
							return{};
						Tool::at_scope_exit tem([&,this]()
						{
							cp->Unmap(ptr, 0);
						});
						return{ t(DMS.pData, DBD.ByteWidth) };
					}
				}
				return{};
			}
			operator bool() const { return ptr != nullptr; }
		};

		struct index : buffer
		{
			size_t offset;
			DXGI_FORMAT format;
			bool create_index(Implement::resource_ptr& rp, Purpose::buffer_purpose bp, const void* data, size_t buffer_size, DXGI_FORMAT DF)
			{
				if (buffer::create(rp, bp.usage, bp.cpu_flag, bp.additional_bind | D3D11_BIND_FLAG::D3D11_BIND_INDEX_BUFFER, data, buffer_size, 0, 0))
				{
					offset = 0;
					format = DF;
					return true;
				}
				return false;
			}
			template<typename T>
			bool create_index(Implement::resource_ptr& rp, Purpose::buffer_purpose bp, T* data, size_t size)
			{
				return create_index(rp, bp, data, sizeof(T)*size, DXGI::data_format<T>::format);
			}
		};

		struct vertex : buffer
		{
			size_t offset;
			size_t element_size;
			std::vector<D3D11_INPUT_ELEMENT_DESC> desc;
			bool create_vertex(Implement::resource_ptr& rp, Purpose::buffer_purpose bp, const void* data, size_t element_size, size_t buffer_size, std::vector<D3D11_INPUT_ELEMENT_DESC> des)
			{
				if (buffer::create(rp, bp.usage, bp.cpu_flag, bp.additional_bind | D3D11_BIND_FLAG::D3D11_BIND_VERTEX_BUFFER, data, buffer_size, 0, 0))
				{
					offset = 0;
					this->element_size = element_size;
					desc = std::move(des);
					return true;
				}
				return false;
			}

			template<typename T>
			bool create_vertex(Implement::resource_ptr& rp, Purpose::buffer_purpose bp, T* data, size_t size, std::vector<D3D11_INPUT_ELEMENT_DESC> des)
			{
				return create_vertex(rp, bp, data, sizeof(T), sizeof(T) * size, std::move(des));
			}
		};

		bool create_index_vertex_buffer(Implement::resource_ptr& rp, Purpose::buffer_purpose bp, 
			const void* data,  size_t buffer_size,
			index& ind, vertex& ver,
			size_t index_offset, DXGI_FORMAT format,
			size_t vertex_offset, size_t element_size, std::vector<D3D11_INPUT_ELEMENT_DESC> layout
		);

		struct constant_value : buffer
		{
			bool create_value(Implement::resource_ptr& rp, Purpose::buffer_purpose bp, const void* data, size_t buffer_size)
			{
				return buffer::create(rp, bp.usage, bp.cpu_flag, bp.additional_bind | D3D11_BIND_FLAG::D3D11_BIND_CONSTANT_BUFFER, data, buffer_size, 0, 0);
			}
			template<typename T>
			bool create_value(Implement::resource_ptr& rp, Purpose::buffer_purpose bp, T* data)
			{
				return create_value(rp, bp, data, sizeof(T));
			}
		};

		struct structured_value : buffer
		{
			bool create_value(Implement::resource_ptr& rp, Purpose::buffer_purpose bp, const void* data,  size_t element_size, size_t buffer_size)
			{
				return buffer::create(rp, bp.usage, bp.cpu_flag, bp.additional_bind | D3D11_BIND_FLAG::D3D11_BIND_UNORDERED_ACCESS, data, buffer_size, D3D11_RESOURCE_MISC_BUFFER_STRUCTURED, element_size);
			}
			template<typename T>
			bool create_value(Implement::resource_ptr& rp, Purpose::buffer_purpose bp, T* data, size_t size)
			{
				return create_value(rp, bp, data, sizeof(T), sizeof(T) * size);
			}
		};

		struct input_layout
		{
			Implement::layout_ptr ptr;
			bool create_input_layout(Implement::resource_ptr& rp, const binary& b, vertex* ver, size_t index);
		};

		Implement::resource_view_ptr cast_resource(Implement::resource_ptr& rp, const Implement::texture2D_ptr& pt);

		Implement::resource_view_ptr cast_resource(Implement::resource_ptr& rp, const Implement::texture1D_ptr& pt);

		Implement::resource_view_ptr cast_resource(Implement::resource_ptr& rp, const Implement::texture3D_ptr& pt);


		struct pixel_creater
		{
			vertex vec[D3D11_IA_VERTEX_INPUT_RESOURCE_SLOT_COUNT];
			index ind;
			input_layout il;
			binary vshader_binary;
			Implement::vshader_ptr vshader;
			Implement::gshader_ptr gshader;
			D3D11_PRIMITIVE_TOPOLOGY primitive = D3D11_PRIMITIVE_TOPOLOGY::D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
			vertex& operator [](size_t ind) 
			{
				return vec[ind]; 
			}
			bool load_vshader(Implement::resource_ptr& rp, std::u16string path) 
			{
				vshader = nullptr;
				if (vshader_binary.load_file(path))
					return rp->CreateVertexShader(vshader_binary, vshader_binary.size(), nullptr, &vshader) == S_OK;
				return false;
			}
			bool load_gshader(Implement::resource_ptr& rp, std::u16string path)
			{
				gshader = nullptr;
				binary tem;
				if (tem.load_file(path))
					return rp->CreateGeometryShader(tem, tem.size(), nullptr, &gshader) == S_OK;
				return false;
			}
			bool update(Implement::resource_ptr& rp)
			{
				return il.create_input_layout(rp, vshader_binary, vec, D3D11_IA_VERTEX_INPUT_RESOURCE_SLOT_COUNT);
			}
			bool create_index_vertex(size_t count, Implement::resource_ptr& rp, Purpose::buffer_purpose bp,
				const void* data, size_t buffer_size,
				size_t index_offset, DXGI_FORMAT format,
				size_t vertex_offset, size_t element_size, std::vector<D3D11_INPUT_ELEMENT_DESC> layout
			)
			{
				return create_index_vertex_buffer(rp, bp, data, buffer_size, ind, vec[count], index_offset, format, vertex_offset, element_size, std::move(layout));
			}
			struct range { UINT start, count; }index_r, vertex_r, instance_r;
			void set_index_range(UINT s, UINT e) { index_r = range{ s, e }; }
			void set_vertex_range(UINT s, UINT e) { vertex_r = range{ s, e }; }
			void set_instance_range(UINT s, UINT e) { instance_r = range{ s, e }; }
			void draw(Implement::context_ptr& cp);
		};

		struct material
		{
			Implement::pshader_ptr pshader;
		public:
			bool load_p(Implement::resource_ptr& rp, std::u16string path)
			{
				pshader = nullptr;
				binary tem;
				if (tem.load_file(path))
					return rp->CreatePixelShader(tem, tem.size(), nullptr, &pshader) == S_OK;
				return false;
			}
			void draw(Implement::context_ptr& cp)
			{
				cp->PSSetShader(pshader, nullptr, 0);
			}
		};


		//Implement::layout_ptr create_input_layout(Implement::resource_ptr& rp, const binary& b, const std::vector<vertex>& ver);

		struct p
		{
			D3D11_USAGE usage;
			UINT cpu_flag;
			UINT additional_bind;
		};

		namespace Property
		{
			struct input
			{
				static constexpr D3D11_USAGE usage = D3D11_USAGE::D3D11_USAGE_DYNAMIC;
				static constexpr UINT cpu_flag = D3D11_CPU_ACCESS_FLAG::D3D11_CPU_ACCESS_WRITE;
				static constexpr UINT additional_bind = 0;
			};
			struct constant
			{
				static constexpr D3D11_USAGE usage = D3D11_USAGE::D3D11_USAGE_IMMUTABLE;
				static constexpr UINT cpu_flag = 0;
				static constexpr UINT additional_bind = 0;
			};
			struct transfer
			{
				static constexpr D3D11_USAGE usage = D3D11_USAGE::D3D11_USAGE_STAGING;
				static constexpr UINT cpu_flag = UINT(D3D11_CPU_ACCESS_FLAG::D3D11_CPU_ACCESS_READ) | (UINT)(D3D11_CPU_ACCESS_FLAG::D3D11_CPU_ACCESS_WRITE);
				static constexpr UINT additional_bind = 0;
			};
			struct output
			{
				static constexpr D3D11_USAGE usage = D3D11_USAGE::D3D11_USAGE_DEFAULT;
				static constexpr UINT cpu_flag = 0;
				static constexpr UINT additional_bind = D3D11_BIND_FLAG::D3D11_BIND_STREAM_OUTPUT | D3D11_BIND_FLAG::D3D11_BIND_UNORDERED_ACCESS;
			};
		}

		struct layout_scription
		{
			size_t layout_count;
			using fun_type = void(*)(D3D11_INPUT_ELEMENT_DESC*, size_t solt);
			fun_type scription;
		};

		struct geometry : buffer
		{

			struct index_range
			{
				size_t offset;
				size_t size;
				DXGI_FORMAT DF;
				index_range& operator=(const index_range&) = default;
				index_range(size_t o, size_t s, DXGI_FORMAT D) :offset(o), size(s), DF(D) {}
				index_range() {}
			}index;

			struct vertex_range
			{
				size_t offset;
				size_t size;
				size_t count;
				size_t step;
				using fun_type = void(*)(D3D11_INPUT_ELEMENT_DESC*, size_t solt);
				fun_type scription;
				vertex_range& operator=(const vertex_range&) = default;
				vertex_range(size_t o, size_t s, size_t c, size_t p, fun_type f) :offset(o), size(s), count(c), step(p), scription(f){}
				vertex_range() {}
			}vertex;

			D3D11_PRIMITIVE_TOPOLOGY primitive;

			bool create_implement(
				Implement::resource_ptr& rp, D3D11_USAGE usage, UINT cpu_flag, UINT bind_flag,
				void* index_data, size_t index_size, DXGI_FORMAT DF,
				void* vertex_data, size_t setp, size_t vertex_size, size_t count, void(*scription)(D3D11_INPUT_ELEMENT_DESC*, size_t solt)
			);

			template<typename property, typename index_type, typename vertex_type, typename layout_type>
			bool create(Implement::resource_ptr& rp, property p, index_type* index_data, size_t index_size, vertex_type* vertex_data, size_t vertex_size, layout_type layout)
			{
				return geometry::create_implement(
					rp, property::usage, property::cpu_flag, property::additional_bind,
					index_data, sizeof(index_type) * index_size, DXGI::data_format<index_type>::format,
					vertex_data, sizeof(vertex_type), sizeof(vertex_type) * vertex_size, layout_type::value, layout
				);
			}

		};

		using geometry_ptr = std::shared_ptr<geometry>;
		using geometry_weak_ptr = std::weak_ptr<geometry>;

		struct instance : buffer
		{
			struct vertex_range
			{
				size_t size;
				size_t count;
				size_t step;
				using fun_type = void(*)(D3D11_INPUT_ELEMENT_DESC*, size_t solt);
				fun_type scription;
				vertex_range& operator=(const vertex_range&) = default;
				vertex_range(size_t s, size_t c, size_t p, fun_type f) : size(s), count(c), step(p), scription(f) {}
				vertex_range() {}
			}vertex;
			bool create_implement(
				Implement::resource_ptr& rp, D3D11_USAGE usage, UINT cpu_flag, UINT bind_flag,
				void* vertex_data, size_t setp, size_t vertex_size, size_t count, void(*scription)(D3D11_INPUT_ELEMENT_DESC*, size_t solt)
			);
			template<typename property, typename vertex_type, typename layout_type>
			bool create(Implement::resource_ptr& rp, property p,vertex_type* vertex_data, size_t vertex_size, layout_type layout)
			{
				return instance::create_implement(
					rp, property::usage, property::cpu_flag, property::additional_bind,
					vertex_data, sizeof(vertex_type), sizeof(vertex_type) * vertex_size, layout_type::value, layout
				);
			}
		};

		using instance_ptr = std::shared_ptr<instance>;
		using instance_weak_ptr = std::weak_ptr<instance>;



		


		/*
		struct draw_data
		{
			Tool::variant<geometry_ptr, geometry_weak_ptr> geo;
			Tool::variant<instance_ptr, instance_weak_ptr> ins[D3D11_IA_VERTEX_INPUT_RESOURCE_SLOT_COUNT - 1];
			struct range { size_t start, count; } index, vertex, instance;
		public:
			bool draw(Implement::context_ptr& cp, Implement::resource_ptr& rp, void* vshader_data, size_t vshader_size);
			void set_index_range(size_t s, size_t e) { index = range{ s, e }; }
			void set_vertex_range(size_t s, size_t e) { vertex = range{ s, e }; }
		};
		*/











		/*
		struct buffer
		{
			buffer_ptr ptr;
			uint64_t buffer_vision;

			uint64_t update() { return ++buffer_vision; }
			bool check_update(uint64_t& i) const
			{
				bool re = (i != buffer_vision);
				i = buffer_vision;
				return re;
			}
			uint64_t vision() const { return buffer_vision; }
			buffer() : ptr(nullptr), buffer_vision(0) {}
			buffer(const buffer_ptr& bp) : ptr(bp), buffer_vision(1) {}
			buffer(buffer&& b) : ptr(b.ptr), buffer_vision(b.buffer_vision)
			{
				b.ptr.Release();
				b.buffer_vision = 0;
			}
		};

		namespace Property
		{
			struct input 
			{
				static constexpr D3D11_USAGE usage = D3D11_USAGE::D3D11_USAGE_DYNAMIC;
				static constexpr UINT cpu_flag = D3D11_CPU_ACCESS_FLAG::D3D11_CPU_ACCESS_WRITE;
				static constexpr UINT additional_bind = 0;
			};
			struct constant 
			{
				static constexpr D3D11_USAGE usage = D3D11_USAGE::D3D11_USAGE_IMMUTABLE;
				static constexpr UINT cpu_flag = 0;
				static constexpr UINT additional_bind = 0;
			};
			struct transfer
			{
				static constexpr D3D11_USAGE usage = D3D11_USAGE::D3D11_USAGE_STAGING;
				static constexpr UINT cpu_flag = UINT(D3D11_CPU_ACCESS_FLAG::D3D11_CPU_ACCESS_READ) | (UINT)(D3D11_CPU_ACCESS_FLAG::D3D11_CPU_ACCESS_WRITE);
				static constexpr UINT additional_bind = 0;
			};
			struct output
			{
				static constexpr D3D11_USAGE usage = D3D11_USAGE::D3D11_USAGE_DEFAULT;
				static constexpr UINT cpu_flag = 0;
				static constexpr UINT additional_bind = D3D11_BIND_FLAG::D3D11_BIND_STREAM_OUTPUT | D3D11_BIND_FLAG::D3D11_BIND_UNORDERED_ACCESS;
			};
		}

		namespace Purpose
		{
			struct vertex 
			{
				struct scription
				{
					//void 
				};
			};
			struct index 
			{
				struct scription
				{

				};
			};

			struct value {};
			struct transfer {};
		}

		namespace Implement
		{
			struct vertex_data
			{
				Implement::buffer_ptr ptr;
				size_t input_layout_count;
				int64_t vision;
				void(*scription)(D3D11_INPUT_ELEMENT_DESC*, size_t solt) = nullptr;
				operator bool() const { return ptr != nullptr; }
				bool create_buffer_implement
				(
					Implement::resource_ptr& rp, D3D11_USAGE usage, UINT cpu_flag, UINT additional_bind,
					const void* data, size_t vertex_size, size_t vertex_count, size_t layout_count, void(*scription)(D3D11_INPUT_ELEMENT_DESC*, size_t solt)
				);
			};
		}

		namespace Implement
		{
			struct layout_scription
			{
				size_t layout_count;
				void(*scription)(D3D11_INPUT_ELEMENT_DESC*, size_t solt) = nullptr;
			};


			struct geometry_store
			{
				Implement::buffer_ptr ptr;
				D3D11_PRIMITIVE_TOPOLOGY primitive;

				struct index_scription
				{
					size_t offset;
					size_t size;
					DXGI_FORMAT format;
				} index_scr;

				struct vertex_scription
				{
					size_t offset;
					size_t size;
					layout_scription layout;
				} vertex_scr;

				int64_t vision;
				
				operator bool() const { return ptr != nullptr; }

				bool create_buffer_implement
				(
					Implement::resource_ptr& rp, D3D11_USAGE usage, UINT cpu_flag, UINT additional_bind,
					const void* data, size_t index_offset, size_t index_size, DXGI_FORMAT index_format,
					size_t vertex_offset, size_t vertex_size, size_t layout_count, void(*scription)(D3D11_INPUT_ELEMENT_DESC*, size_t solt)
				);

				bool create_buffer_implement
				(
					Implement::resource_ptr& rp, D3D11_USAGE usage, UINT cpu_flag, UINT additional_bind,
					const void* index_data, size_t index_size, DXGI_FORMAT index_format,
					const void* vertex_data, size_t vertex_size, size_t layout_count, void(*scription)(D3D11_INPUT_ELEMENT_DESC*, size_t solt)
				);
			};
		}

		template<typename property = Property::constant> struct geometry : Implement::geometry_store
		{
		public:
			template<typename T, typename K, typename L>
			bool create(Implement::resource_ptr& rp, T* index_data, size_t index_size, K* vertex_data, size_t vertex_size, L l)
			{
				return Implement::geometry_store::create_buffer_implement(
					rp, property::usage, property::cpu_flag, property::additional_bind,
					index_data, sizeof(T) * index_size, DXGI::data_format<T>::format,
					vertex_data, sizeof(K) * vertex_size, L::value, l
				);
			}
		};

		template<typename property = Property::constant>
		using geometry_ptr = std::shared_ptr<geometry<property>>;

		template<typename property = Property::constant>
		using geometry_weak_ptr = std::weak_ptr<geometry<property>>;

		namespace Implement
		{
			struct instance_store
			{
				Implement::buffer_ptr ptr;
				layout_scription ls;
				operator bool() const { return ptr != nullptr; }
				bool create_buffer_implement
				(
					resource_ptr& rp, D3D11_USAGE usage, UINT cpu_flag, UINT additional_bind,
					const void* instance_data, size_t instance_size, size_t layout_count, void(*scription)(D3D11_INPUT_ELEMENT_DESC*, size_t solt)
				);
				//void create_input_element(D3D11_INPUT_ELEMENT_DESC*, size_t solt) {return ls.}
			};
		}

		template<typename property = Property::constant> struct instance : Implement::instance_store
		{
		public:
			template<typename T, typename K>
			bool create(Implement::resource_ptr& rp, T* data, size_t s, K k)
			{
				return Implement::instance_store::create_buffer_implement(
					rp, property::usage, property::cpu_flag, property::additional_bind,
					data, sizeof(T)*s, K::value, k
				);
			}
		};

		template<typename property = Property::constant>
		using instance_ptr = std::shared_ptr<instance<property>>;

		template<typename property = Property::constant>
		using instance_weak_ptr = std::weak_ptr<instance<property>>;

		namespace Implement
		{
			struct index_data
			{
				Implement::buffer_ptr ptr;
				DXGI_FORMAT format;
			};
		}

		template<typename purpose, typename property = Property::constant>
		struct buffer : purpose::scription
		{
			Implement::buffer_ptr ptr;
			typename purpose::scription scr;
		public:
			operator bool() const { return ptr != nullptr; }
			template<typename T, typename K, typename L>
			bool create_array(Implement::resource_ptr& rp, const T* data, size_t s, L l)
			{
				D3D11_BUFFER_DESC DBD
				{
					static_cast<UINT>(sizeof(T) * p.size()),
					property::usage,
					purpose::bind_flag | purpose::additional_bind,
					property::cpu_flag,
					std::is_same<purpose, Purpose::vertex>::value ? 0 : D3D11_RESOURCE_MISC_BUFFER_STRUCTURED,
					std::is_same<purpose, Purpose::vertex>::value ? 0 : static_cast<UINT>(sizeof(T))
				};
				D3D11_SUBRESOURCE_DATA DSD { static_cast<void>(data), 0, 0 };
				ptr = nullptr;
				HRESULT re = rp->CreateBuffer(&DBD, &DSD, ptr);
				if (re == S_OK)
				{
					purpose{}(scr, p.data(), p.size(), l);
					return true;
				}
				return false;
			}*/

			//bool create_array
			/*
			template<typename T, typename K, typename L>
			auto create(const std::vector<T, K>& p, L l)->std::enable_if_t<Tmp::is_one_of<purpose, Purpose::buff>>
			{
				create(p.data(), p.size(), l);
			}
			template<typename T, typename L>
			auto create(const std::vector<>)
			*/
		//};

/*
		struct vertex_buffer : Implement::buffer
		{
			size_t input_layout_count = 0;
			size_t vertex_size;
			void(*scription)(D3D11_INPUT_ELEMENT_DESC*, size_t solt) = nullptr;
			vertex_buffer() {}
			vertex_buffer(
				const Implement::buffer_ptr& bf, size_t ilc, size_t vs,
				void(*scr)(D3D11_INPUT_ELEMENT_DESC*, size_t solt)
			) : Implement::buffer(bf), input_layout_count(ilc), vertex_size(vs), scription(scr) {}
			vertex_buffer(vertex_buffer&& vd) : Implement::buffer(std::move(vd)), scription(vd.scription)
			{
				vd.scription = nullptr;
			}
		};

		struct index_buffer : Implement::buffer
		{
			DXGI_FORMAT format;
			UINT offset;
			index_buffer(const Implement::buffer_ptr& bp, DXGI_FORMAT f, size_t o) :Implement::buffer(bp), format(f), offset(static_cast<UINT>(o)) {}
		};

		struct vertex_pool
		{

			struct element_data
			{
				using store_ref = std::shared_ptr<vertex_buffer>;
				using weak_ref = std::weak_ptr<vertex_buffer>;

				Tool::variant<store_ref, weak_ref> ptr;
				uint64_t vision = 0;
				bool need_change = false;
				vertex_buffer& get_element() { return ptr.able_cast<store_ref>() ? *ptr.cast<store_ref>() : *(ptr.cast<weak_ref>().lock()); }
				operator bool() const;
				element_data& operator=(store_ref sr);
				bool need_update();
				void clear() 
				{
					vision = 0;
					ptr = {};
					need_change = true;
				}
			};

			element_data element[D3D11_IA_VERTEX_INPUT_RESOURCE_SLOT_COUNT];
			Tool::variant<std::shared_ptr<index_buffer>, std::weak_ptr<index_buffer>> index_ptr;
			std::map<binary::weak_ref, Implement::layout_ptr> layout_state;

			D3D11_PRIMITIVE_TOPOLOGY primitive = D3D11_PRIMITIVE_TOPOLOGY::D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
			

			Implement::resource_ptr res_ptr;

			struct range
			{
				size_t start;
				size_t count;
			};

			range vertex;
			range instance;
			range index;

			bool update(Implement::context_ptr& cp, binary& b);
			void clear();
			bool set_resource(const Implement::resource_ptr& rp) { clear(); res_ptr = rp; return res_ptr != nullptr; }
			void set_vertex(size_t start, size_t count) { vertex = range{ start, count }; }
			void set_instance(size_t start, size_t count) { instance = range{ start, count }; }
			void set_index(size_t start, size_t count) { index = range{ start, count }; }

			HRESULT create_vertex(size_t solt, void* data, size_t type_size, size_t data_size, size_t vertex_size, size_t layout_count, void(*scription)(D3D11_INPUT_ELEMENT_DESC*, size_t solt));

			void set_primitive(D3D11_PRIMITIVE_TOPOLOGY p) { primitive = p; }

			template<typename type, typename inpu>
			HRESULT create_vertex(size_t solt,  type* t, size_t s, inpu)
			{
				return create_vertex(solt, t, sizeof(type), sizeof(type) * s, sizeof(type), inpu::value, &inpu::create_input_element_desc);
			}

			HRESULT create_index(void* data, size_t data_size, DXGI_FORMAT DF);
		};

		struct const_buffer
		{

		};

		struct pipe_line
		{
			struct v
			{
				using store = Implement::vshader_ptr;
				static HRESULT create(const Implement::resource_ptr& rp, store& s, const binary& b) { return rp->CreateVertexShader(b, b.size(),nullptr, &s); }
			};

			struct p
			{
				using store = Implement::pshader_ptr;
				static HRESULT create(const Implement::resource_ptr& rp, store& s, const binary& b) { return rp->CreatePixelShader(b, b.size(), nullptr, &s); }
			};

			struct g
			{
				using store = Implement::gshader_ptr;
				static HRESULT create(const Implement::resource_ptr& rp, store& s, const binary& b) { return rp->CreateGeometryShader(b, b.size(), nullptr, &s); }
			};

			template<typename T> struct shader_packet
			{
				binary buffer;
				typename T::store ptr;
			public:
				HRESULT load(const Implement::resource_ptr& rp, binary&& b)
				{
					buffer = std::move(b);
					return T::create(rp, ptr, buffer);
				}
				void clear() 
				{
					ptr.Release();
				}
			};

			shader_packet<v> vshader;
			shader_packet<p> pshader;
			shader_packet<g> gshader;

			Implement::resource_ptr res_ptr;
			Implement::raterizer_state_ptr state_ptr;
		public:
			bool set_resource(const Implement::resource_ptr& rp) 
			{ 
				clear();
				res_ptr = rp;
				/*
				if (shader_v_buffer)
				{
					binary tem = std::move(shader_v_buffer);
					load_shader_v(std::move(tem));
				}
				if (shader_p_buffer)
				{
					binary tem = std::move(shader_p_buffer);
					load_shader_p(std::move(tem));
				}
				return res_ptr != nullptr; 
			}
			void clear()
			{
				res_ptr.Release();
				vshader.clear();
				pshader.clear();
				gshader.clear();
				state_ptr.Release();
			}
			bool draw(Implement::context_ptr& cp, /*const_buffer& cb, vertex_pool& vp, size_t vertex_num);
			HRESULT load_shader_v(binary&& b);
			HRESULT load_shader_g(binary&& b);
			HRESULT load_shader_p(binary&& b);
			void Set_Con(Implement::context_ptr& cp)
			{
				//cp->VSSetConstantBuffers

			}
		};

		struct compute
		{
			Implement::resource_ptr res_ptr;
			binary shader_c_buffer;
			Implement::cshader_ptr  shader_c;
			bool set_resource(Implement::resource_ptr ip) { res_ptr = ip; return res_ptr != nullptr; }
			HRESULT load_shader_c(binary&& a)
			{
				shader_c_buffer = std::move(a);
				HRESULT re = res_ptr->CreateComputeShader(shader_c_buffer, shader_c_buffer.size(), nullptr, &shader_c);
				return re;
			}
		};

		struct scene
		{
			Implement::resource_ptr res_ptr;
		public:
		};


			class data
			{
				std::shared_ptr<void> steam;
				static void deleter(void* p) { delete [] p; }
			public:

				class weak_ref
				{
					std::weak_ptr<void> steam;
				public:
					operator bool() const { return !steam.expired(); }
					weak_ref() {}
					weak_ref(const weak_ref&) = default;
					weak_ref(weak_ref&&) = default;
					weak_ref(const data& d) :steam(d.steam) {}
					weak_ref& operator= (const weak_ref&) = default;
					weak_ref& operator= (weak_ref&&) = default;
					weak_ref& operator= (const data& d)
					{
						steam = d.steam;
						return *this;
					}
					auto lock() const { return steam.lock(); }
				};

				data() {}
				data(size_t da) : steam(new char[da + sizeof(size_t)], deleter)
				{
					auto scr = reinterpret_cast<size_t*>(steam.get());
					*scr = da;
				}
				data(const data& da) = default;
				data(data&& da) = default;
				data& operator=(const data& da) = default;
				data& operator=(data&& da) = default;
				void reset()
				{
					steam.reset();
				}
				operator bool() const { return static_cast<bool>(steam); }
				~data() {}
				operator void*() const { return static_cast<void*>(static_cast<char*>(steam.get()) + sizeof(size_t)); }
				operator char*() const { return static_cast<char*>(steam.get()) + sizeof(size_t); }
				size_t size() const 
				{
					return steam ? *reinterpret_cast<size_t*>(steam.get()) : 0;
				}
			};
		}

		struct buffer_state
		{
			bool change = false;
		};

		template<typename property, typename ...usetype> class buffer
		{
			bool change = false;
		};
		
		
		class vertex_buffer
		{
		protected:
			Implement::buffer data;
			size_t ver_size = 0;
			size_t ver_numb = 0;
			operator ID3D11Buffer *() { return data; }
			friend class pipe_line;
		public:
			operator bool() const {return data!=nullptr; }
			vertex_buffer() {}
			vertex_buffer(vertex_buffer&& vb) : data(std::move(vb.data)), ver_size(vb.ver_size), ver_numb(vb.ver_numb) 
			{
				vb.ver_size = 0;
				vb.ver_numb = 0;
			}
			vertex_buffer(Implement::buffer da, size_t vs, size_t vn) : data(da), ver_size(vs), ver_numb(vn) {}

			size_t size() const { return ver_size; }
			size_t num() const { return ver_numb; }
		};

		namespace Property
		{
			struct constant
			{
				static constexpr D3D11_USAGE USAGE = D3D11_USAGE::D3D11_USAGE_IMMUTABLE;
				static constexpr UINT CPU_ACCESS = 0;
			};

			struct input
			{
				static constexpr D3D11_USAGE USAGE = D3D11_USAGE::D3D11_USAGE_DYNAMIC;
				static constexpr UINT CPU_ACCESS = D3D11_CPU_ACCESS_FLAG::D3D11_CPU_ACCESS_WRITE;
			};

			struct readable
			{
				static constexpr D3D11_USAGE USAGE = D3D11_USAGE::D3D11_USAGE_STAGING;
				static constexpr UINT CPU_ACCESS = static_cast<D3D11_CPU_ACCESS_FLAG>(D3D11_CPU_ACCESS_FLAG::D3D11_CPU_ACCESS_WRITE | D3D11_CPU_ACCESS_FLAG::D3D11_CPU_ACCESS_READ);
			};
		}

		namespace Component
		{
			template<typename T> struct is_component : std::false_type {};

			struct shader
			{
				struct vertex
				{
					using store_type = Implement::vshader;
					static HRESULT LoadShader(store_type& s,Implement::resource& re, Implement::data& da)
					{
						return re->CreateVertexShader(da, da.size(), nullptr, &s);
					}
				};7

				struct pixel
				{
					using store_type = Implement::pshader;
					static HRESULT LoadShader(store_type& s, Implement::resource& re, Implement::data& da)
					{
						return re->CreatePixelShader(da, da.size(), nullptr, &s);
					}
				};
			};

			struct index
			{

			};
		}

		template<typename pro, typename ...bind_type> class buffer
		{	
			Implement::buffer ptr;
			size_t size;
			std::tuple<typename bind_type::scription...> scr;
			friend class pipe_line;
		public:
			buffer() {}
			operator bool() const { return ptr != nullptr; }
		};

		namespace Implement
		{
			template<typename T> struct data_format;
			template<> struct data_format<float2>
			{
				static constexpr DXGI_FORMAT format = DXGI_FORMAT::DXGI_FORMAT_R32G32_FLOAT;
			};
			template<> struct data_format<float3>
			{
				static constexpr DXGI_FORMAT format = DXGI_FORMAT::DXGI_FORMAT_R32G32B32_FLOAT;
			};
			template<> struct data_format<float4>
			{
				static constexpr DXGI_FORMAT format = DXGI_FORMAT::DXGI_FORMAT_R32G32B32A32_FLOAT;
			};
		}
		
		template<typename Property_t> class vertex : public vertex_buffer
		{
			void(*layout_creater)(std::vector<D3D11_INPUT_ELEMENT_DESC>& v, size_t slot);
		public:
			using vertex_buffer::vertex_buffer;
			template<typename D, typename layout> HRESULT create(const Implement::resource& re, D* da, size_t s, layout)
			{
				data.reset();
				static_assert(std::is_pod<D>::value, "vertex creating only receive pod struct");
				D3D11_BUFFER_DESC tem_b{
					sizeof(D) * s,
					Property_t::USAGE,
					static_cast<UINT>(D3D11_BIND_FLAG::D3D11_BIND_VERTEX_BUFFER),
					Property_t::CPU_ACCESS,
					static_cast<UINT>(0x0L),
					0
				};
				D3D11_SUBRESOURCE_DATA tem_s{ da, 0, 0 };
				HRESULT re = re->CreateBuffer(&tem_b, &tem_s, &data);
				if (SUCCEEDED(re))
				{
					layout_creater = layout::create_input_element_desc;
					ver_size = sizeof(da);
					ver_numb = s;
				}
				return re;
			}
		};

		namespace Components
		{
			struct vertex_s
			{
				using store_type = Implement::vshader;
				static HRESULT LoadShader(store_type& s,Implement::resource& re, Implement::data& da)
				{
					return re->CreateVertexShader(da, da.size(), nullptr, &s);
				}
			};

			struct pixel_s
			{
				using store_type = Implement::pshader;
				static HRESULT LoadShader(store_type& s, Implement::resource& re, Implement::data& da)
				{
					return re->CreatePixelShader(da, da.size(), nullptr, &s);
				}
			};
		}

		template<typename shader_t>
		class shader
		{
			Implement::data da;
			typename shader_t::store_type sh;
			friend class pipe_line;
		public:
			shader(const shader&) = default;
			shader(shader&& s) = default;
			shader() = default;
			shader& operator=(const shader&) = default;
			shader& operator=(shader&&) = default;

			shader(const Implement::resource& re, Implement::data da)
			{
				load_binary(re, da);
			}

			operator bool() const { return shader != nullptr; }

			HRESULT load_binary(const Implement::resource& re, const Implement::data& d)
			{
				sh = nullptr;
				da = d;
				return shader_t::load_binary(sh, re, da);
			}
		};


		class pipe_line
		{
			using vs_t = shader<Component::shader<Component::shader::pixel>>;
			using ps_t = shader<Component::pixel_s>;

			vs_t vs;
			ps_t ps;

			static thread_local std::vector<D3D11_INPUT_ELEMENT_DESC> input_element_buffer;

		public:

			pipe_line() {}

			pipe_line(pipe_line&&) = default;
			pipe_line(const pipe_line&) = default;
			
			pipe_line& operator= (pipe_line&& pl) = default;
			pipe_line& operator= (const pipe_line& pl) = default;

			vs_t& v_shader() { return vs; }
			ps_t& p_shader() { return ps; }

			void draw(Implement::context& c, ID3D11RenderTargetView* pView, ID3D11DepthStencilView* pDepthView, const vertex_layout& vl, vertex_buffer& vb)
			{
				
				unsigned int stride = static_cast<UINT>(vb.size());
				unsigned int offset = 0;
				HRESULT last = GetLastError();
				
				c->IASetInputLayout(vl.lay);
				ID3D11Buffer * const adress[] = { vb };
				last = GetLastError();
				c->IASetVertexBuffers(0, 1, adress, &stride, &offset);
				last = GetLastError();
				c->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY::D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
				last = GetLastError();
				c->VSSetShader(v.shader, nullptr, 0);
				last = GetLastError();
				c->PSSetShader(p.shader, nullptr, 0);
				last = GetLastError();
				c->Draw(6, 0);
				last = GetLastError();
			}
			
		};
		*/
	}
}
