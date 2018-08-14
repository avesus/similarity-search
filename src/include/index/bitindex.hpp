#pragma once

#include <unordered_map>
#include <vector>

#include "../index.hpp"

#include "../utils/hashers.hpp"

namespace ss {

	using namespace std;

	template<class DataType>
	class BitIndex : public Index<DataType> {

		using KeyType = unsigned long long;
	
	protected:
		/**
		 * if you want to make data 0-means in each dimension, then you should invoke #initilize_mean() in #train(); 
		 * if not, let it alone and means are initilized with default value 0.
		 */
		vector<DataType >							_means;
		unordered_map<KeyType, vector<int>, ss::SSHasher<KeyType > > 		_hash_map;
		vector<vector<DataType > >						_projectors;
	public:	
		BitIndex(parameter& para) : 
			Index<DataType >(para), 
			_means(para.dim, 0),
			_projectors(para.num_bit, vector<DataType >(para.dim, 0)) {}

		unordered_map<KeyType, vector<int>, SSHasher<KeyType > > & getIndexMap() { return _hash_map;  }

		virtual void train(const lshbox::Matrix<DataType >& ) = 0; 

		virtual void add  (const lshbox::Matrix<DataType >& data) {

			for (int i=0; i<data.getSize(); ++i) {
				KeyType hash_val = hash_data(data[i], i);
				_hash_map[hash_val].push_back(i);
			}
		}

		virtual KeyType hash_data (const DataType * data, int id)	{ return this->quantize(data, 0); }
		virtual KeyType hash_query(const DataType * query) 		{ return this->hash_data(query, -1); }

	protected:

		virtual KeyType quantize(const DataType * data, DataType quantizor) {
			KeyType value = 0;
			for (int i=0; i<this->_para.num_bit; i++) {
				DataType quantization =  ss::diff_product(data,  _means.data(), _projectors[i].data(), this->_para.dim ) ;	
				value <<= 1;
				value |= (quantization>quantizor) ? 1 :0;
			}
			return value;
		}

		void initilize_means(const lshbox::Matrix<DataType >& train_data) {

			vector<long double> sum(this->_para.dim, .0);	// use long double to make sure presision
			for (int i=0; i < train_data.getSize(); i++) {	// one vector by one vector cache-friendly
				for( int d = 0; d < this->_para.dim; d++ ) {
					sum[d] += train_data[i][d] ;
				}
			}
			for (int d = 0; d < this->_para.dim; d++) {
				this->_means[d]  = sum[d] / train_data.getSize();
			}
		}

	};
	
}