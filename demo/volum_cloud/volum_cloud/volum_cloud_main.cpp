#include <iostream>
using namespace std;

#include <limits>
#include <random>
#include "../../../po.h"
#include "../../../gui/dx11/dx11_form.h"
#include "form_define.h"
#include "test_plugin.h"
#include <fstream>
#include <algorithm>

struct A { A() { cout << "init A" << endl; } ~A() { cout << "delete A" << endl; } };

template<typename T> struct AT : PO::Tool::inherit_t<T> {

};

int main()
{
	AT<int> data;
	PO::context con;
	auto fo = con.create_frame(PO::frame<DX11_Test_Form>{});
	fo.lock([](auto& ui){
		//ui.depute_create_plugin(PO::plugin<test_plugin>{});
	});
	con.wait_all_form_close();
	system("pause");
}