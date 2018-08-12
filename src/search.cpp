#include <iostream>
#include <vector>

#include <boost/program_options.hpp>

#include "include/parameters.hpp"
#include "include/index.hpp"
#include "include/index/srp.hpp"
#include "include/query/hrquery.hpp"

#include "include/bench/bencher.h"
#include "include/bench/benchrecord.h"

#include "include/matrix.h"
#include "include/utils/util.hpp"

using namespace std;
using namespace ss;
using namespace lshbox;


template <class DataType, class IndexType, class Querytype>
int execuate(parameter& para);

void load_options(int argc, char** argv, parameter & para);


int main(int argc, char** argv) {
	using DataType = float;
	parameter para;
	load_options(argc, argv, para);
	execuate<DataType, SRPIndex<DataType >, HRQuery<DataType > >(para);	
}


void load_options(int argc, char** argv, parameter& para) {
	
	namespace po = boost::program_options;

	po::options_description opts("Allowed options");
	opts.add_options()
		("help,h", "help info")

		("num_bit,l",		po::value<int >(&para.num_bit)->default_value(32)  , "num of hash bit")
		("topK,k",		po::value<int >(&para.topK)->default_value(20)     , "size of result set")
		("num_thread",		po::value<int >(&para.num_thread)->default_value(1), "num thread")
		("dim,d", 		po::value<int >(&para.dim)->default_value(-1)      , "origin dimension of data")
		("transformed_dim",	po::value<int >(&para.transformed_dim)->default_value(0)      , "origin dimension of data")
		
		("train_data,t",  	po::value<string >(&para.train_data),   "data for training")
		("base_data,b",		po::value<string >(&para.base_data) ,   "data saved in index")
		("query_data,q",	po::value<string >(&para.query_data),   "data for query")
		("ground_truth,g",	po::value<string >(&para.ground_truth), "ground truth")
	;

	po::variables_map vm;
	po::store(po::parse_command_line(argc, argv, opts), vm);
	po::notify(vm);

	para.map = vm;

	if(vm.count("help")) {
		cout << opts << endl; 
		exit(0);
	}
}

template <class DataType, class IndexType, class QueryType>
int execuate(parameter& para) {

	Bencher truthBencher(para.ground_truth.c_str());	

	lshbox::Matrix<DataType> train_data(para.train_data, para.transformed_dim);
	lshbox::Matrix<DataType> base_data (para.base_data,  para.transformed_dim);
	lshbox::Matrix<DataType> query_data(para.query_data, para.transformed_dim);
	
	para.train_size = train_data.getSize();
	para.base_size  = base_data.getSize();
	para.query_size = query_data.getSize();
	para.dim = train_data.getDim();

	IndexType * index = new IndexType(para);

	index->preprocess_train(train_data);
	index->preprocess_base(base_data);
	index->preprocess_query(query_data);
	index->train(train_data);
	index->add(base_data);

	lshbox::Metric<DataType > metric(para.dim, L2_DIST);	
	typename lshbox::Matrix<DataType >::Accessor accessor(base_data);

	vector<QueryType * > queries;
	for (int i = 0; i < para.query_size; i++) {
		queries.push_back(new QueryType(index, query_data[i], metric, accessor, para ) );
	}

	const char * spliter = ", ";
	cout
		<< "expected items" << spliter
		<< "avg items" << spliter 
		<< "overall time" << spliter 
		<< "avg recall" << spliter 
		<< "avg precision" << spliter
		<< "avg error" 
		<< "\n";

	ss::timer time_recorder;
	for (int numItems = 1; numItems/2 < para.base_size; numItems *=2 ) {
		
		if ( numItems > para.base_size ) 
			numItems = para.base_size;
		
		vector<vector<pair<float, unsigned>>> currentTopK;
		currentTopK.reserve(para.query_size);
		
		vector<unsigned> numItemProbed;
		numItemProbed.reserve(para.query_size);

		for (int i = 0; i <  para.query_size; i++) {
			queries[i]->probeItems(numItems);
			numItemProbed.emplace_back(queries[i]->getNumItemsProbed());
			currentTopK.emplace_back(queries[i]->getSortedTopK());
		}

		// statistic such as recall precision
		Bencher currentBencher(currentTopK, true);
		cout
			<< numItems <<  spliter 
		        << truthBencher.avg_items(numItemProbed) << spliter 
			<< time_recorder.elapsed() << spliter 
		        << truthBencher.avg_recall(currentBencher) << spliter 
		        << truthBencher.avg_precision(currentBencher, numItemProbed) << spliter 
		        << truthBencher.avg_error(currentBencher)
		       	<< "\n";
	}

	delete index;
	for (int i =0 ; i < para.query_size; i++) {
		delete queries[i];
	}

	return 0;
}
