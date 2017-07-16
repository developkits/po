#pragma once
#include "../po_dx11/dx11_frame.h"
#include "../po/renderer.h"
#include "../po_dx11/dx11_renderer.h"
#include "defer_element.h"
#include <typeindex>
namespace PO
{
	namespace Dx11
	{
		struct defer_renderer : simple_renderer
		{
			
			tex2 depth_stencial;
			tex2 color_bufer;
			output_merge_stage oms;

			interface_storage storage_inter;
			defer_element_implement_storage storage_elemnt;

			float4x4 view;
			float4x4 projection;

			element merga;

			depth_stencil_state defer_dss;
			depth_stencil_state merga_dss;

			float time;

			proxy mapping(std::type_index, adapter_interface& ai);

			void init(value_table&);

			void pre_tick(duration);
			void pos_tick(duration);

			/*
			template<typename T> decltype(auto) find(T t) { return storage_inter.find(t, *this);}
			void push_element(const element& ptr);

			std::set<std::type_index> check(const element& ptr) { return storage_elemnt.check(ptr); }
			*/
		};

		class geomerty_render_screen : public geometry_interface
		{
		public:

		};

	}
}