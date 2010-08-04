// TODO: prompt

#include "OptimalNPants.h"
#include "Structures.h"
#include "Flow.h"
#include "IO_Tet.h"
#include "IO_Tet_Adj.h"
#include "Ford_Neighborhood.h"
#include <time.h>
#include <vector>
#undef NDEBUG
#include <assert.h>
#include <string>
#include <iostream>

using namespace std;
using namespace oc3d;
using namespace sgl;

typedef double type_flow;
typedef Edge_Dual<type_flow> Edge;
typedef Graph_List<Edge> Dual;
typedef Edge_Cut<type_flow, Edge> Cut;
typedef Graph_List<Cut> Pants;
typedef Edge_Base Edge_Adj;
typedef Graph_List<Edge_Adj> Dual_Adj; 

void help()
{
	show("Usage:"); //todo
}


void tokenize(const string &str, vector<string> &tokens, const string &delimiters = " ")
{
    string::size_type lastPos = str.find_first_not_of(delimiters, 0);
    string::size_type pos = str.find_first_of(delimiters, lastPos);
    while (string::npos != pos || string::npos != lastPos)
    {
        tokens.push_back(str.substr(lastPos, pos - lastPos));
        lastPos = str.find_first_not_of(delimiters, pos);
        pos = str.find_first_of(delimiters, lastPos);
    }
}

string del_ext(string file)
{
	return file.substr(0, file.find_last_of("."));
}

string args_to_file(vector<string> &tokens, int index)
{
	string file = "";
	for(int i = index; i < tokens.size(); i++)
	{
		file += tokens[i];
		if(i != tokens.size() - 1)
			file += " ";
	}
	return file;
}

bool use_neighbors = true;

Tetrahedrization mesh; 
typedef IO_Tet<Edge, Cut, Dual, Pants> IO_T;
typedef IO_Tet_Adj<Edge, Edge_Adj, Cut, Dual, Dual_Adj, Pants> IO_T_S;
IO_T io_tet(mesh, "");
IO_T_S io_tet_adj(mesh, "");

IO_Tet<Edge, Cut, Dual, Pants> &io() { return use_neighbors ? io_tet_adj : io_tet; }

void init()
{
	TMLib::init();
}
int main(int argc, char *argv[])
{
	init();
	string s;
	
	while(getline(cin, s))
	{
		vector<string> tokens;
		tokenize(s, tokens);

		if(tokens[0] == "neighbors")
		{
			use_neighbors = !use_neighbors;
			show((use_neighbors ? "U" : "Don't u") + toString("se neighborhood for Ford Fulkerson algorithm"));
		}
	    else if(tokens[0] == "load")
		{
			string file = args_to_file(tokens, 1);
			if (mesh.load(file.c_str()) != 0) 
				show("Can't read " + file);
			else
			{
				show("Read " + file);
				string base_file = del_ext(del_ext(file));
				io().set_base_name(base_file);
				show("Base name: " + base_file);
			}
		}
		else if(tokens[0] == "make_dual")
		{
			time_t t1 = clock();
			if(use_neighbors)
			{
				io_tet_adj.make_dual();
				io_tet_adj.graph_to_OFF<Dual_Adj, Edge_Adj>(io_tet_adj.dual_adj, "_adj");
				io_tet_adj.graph_to_OFF<Dual, Edge>(io_tet_adj.dual, "");
			}
			else
			{
				io_tet.make_dual();
				io_tet.graph_to_OFF<Dual, Edge>(io_tet.dual, "");
			}
			time_t t2 = clock();
			show("Time: " + toString((t2-t1)/CLOCKS_PER_SEC));
		}		
		else if(tokens[0] == "opt")
		{
			int num = 0;
			if((tokens.size() == 1 || !fromString(tokens[1], num)) || num >= io().cuts.size())
				help();
			else
			{
				time_t t1 = clock();
				if(use_neighbors)
				{
					Ford_Neighborhood<> neighborhood(io_tet_adj.dual, io_tet_adj.dual_adj, io_tet_adj.get_s(), io_tet_adj.get_t(), io_tet_adj.cuts[num]->cap(), io_tet_adj);
					Cut_Vertices<Edge, Dual> cut_vertices(io_tet_adj.dual);
					typedef OptimalNPants<type_flow, type_flow, Edge, Cut, Dual, Pants, Ford_Neighborhood<> > OptimalNPants;
					OptimalNPants::optimize(num, io_tet_adj, neighborhood, cut_vertices);
					//io_tet_adj.graph_to_OFF<Dual, Edge>(neighborhood.N, "_N"); 
				}
				else
				{
					NoNullCap<Edge> noNull(io_tet.dual.V(), io_tet.get_t()); 
					Fulkerson<type_flow, Edge, NoNullCap<Edge> > fulkerson(io_tet.dual, noNull, io_tet.get_s(), io_tet.get_t());
					Cut_Vertices<Edge, Dual> cut_vertices(io_tet.dual);
					OptimalNPants<>::optimize(num, io_tet, fulkerson, cut_vertices);
				}
				time_t t2 = clock();
				show("Time: " + toString((t2-t1)/CLOCKS_PER_SEC));
			}
		}
		else if(tokens[0] == "load_cut")
		{
			int num = 0;
			if(tokens.size() == 1 || !fromString(tokens[1], num))
				help();
			else
			{
				string file = tokens.size() == 1 ? "" : args_to_file(tokens, 2);
				io().filecut_to_cut(num, file);
				io().init_cut(num);
				show("Cut " + toString(num) + " loaded");
			}
		}
		else if(tokens[0] == "load_thick_cut")
		{
			int num = 0;
			if(tokens.size() == 1 || !fromString(tokens[1], num))
				help();
			else
			{
				string file = tokens.size() == 1 ? "" : args_to_file(tokens, 2);
				io().filecut_to_cut(num, file);
				io().shrink_cut(num, 50);
				io().cut_to_filecut(num, io().base_name + "_cut_" + toString(num) + + "_thin" + ".cut");
				io().init_cut(num);
				show("Cut " + toString(num) + " loaded");
			}
		}
		else if(tokens[0] == "save_cut")
		{
			int num = 0;
			if(tokens.size() == 1 || !fromString(tokens[1], num))
				help();
			else
			{
				string file = tokens.size() == 1 ? "" : args_to_file(tokens, 2);
				io().cut_to_filecut(num, file);
			}
		}
		else if(tokens[0] == "save_off")
			mesh.saveOFFBoundary((io().base_name + ".off").c_str());
		else if(tokens[0] == "init")
		{
			io().init_pants();
		}
		else if(tokens[0] == "exit")
			break;
		else 
			help();

		cout<<"END"<<endl;
	}
}
