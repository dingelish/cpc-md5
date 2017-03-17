/**************************************************************************\
|
|    Copyright (C) 2009 Marc Stevens
|
|    This program is free software: you can redistribute it and/or modify
|    it under the terms of the GNU General Public License as published by
|    the Free Software Foundation, either version 3 of the License, or
|    (at your option) any later version.
|
|    This program is distributed in the hope that it will be useful,
|    but WITHOUT ANY WARRANTY; without even the implied warranty of
|    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
|    GNU General Public License for more details.
|
|    You should have received a copy of the GNU General Public License
|    along with this program.  If not, see <http://www.gnu.org/licenses/>.
|
\**************************************************************************/

#ifndef MAIN_HPP
#define MAIN_HPP

#include <iostream>
#include <vector>
#include <string>

#include <boost/filesystem/operations.hpp>
#include <boost/thread.hpp>

#include <hashutil5/differentialpath.hpp>
#include <hashutil5/booleanfunction.hpp>

using namespace hashutil;
using namespace std;

extern boost::mutex mut;
extern std::string workdir;
class path_container_autobalance;
void dostep(path_container_autobalance& container, bool savetocache = false);

struct md5_backward_thread {
	void md5_backward_differential_step(const hashutil::differentialpath& path, path_container_autobalance& container);
	differentialpath newpath;
	vector<sdr> sdrs;
	bitcondition Qtb[32], Qtm1b[32], Qtm2b[32];
	vector<unsigned> bval;
	uint32 fdiv[32];
	bf_outcome foutcomes[32];	
	std::vector<std::pair<uint32,double> > rotateddiff;
};



class path_container_autobalance {
public:
	path_container_autobalance()
		: modn(1), modi(0), inputfile(), showinputpaths(false)
		, t(0), trange(0), maxweight(1), maxsdrs(1), maxcond(2176), tend(64), ubound(0)
		, estimatefactor(0), noverify(false), includenaf(false)
		, nafestweight(0), halfnafweight(false), newinputpath(false)
		, pathsout(0), size(0), count(0), count_balanced(0)
		, condcount(0), verified(0), verifiedbad(0), maxQ26upcond(1024),minweight(0)
		, fillfraction(0), threads(1)
	{
		for (unsigned k = 0; k < 16; ++k)
			m_diff[k] = 0;
	}

	~path_container_autobalance() 
	{
		cerr << "Autobalance parameters: maxcond=" << maxcond << endl;
		if (!noverify)
			cerr << "Verified: " << verifiedbad << " bad out of " << verified << endl;
	}

	void set_parameters()
	{
		pathsout.clear();
		pathsout.resize(maxcond+1);
		condcount.clear();
		condcount.resize(maxcond+1);
	}

	void estimate(unsigned cond, unsigned amount)
	{
		if (cond > maxcond) return;
		if (ubound == 0) return;
		boost::lock_guard<boost::mutex> lock(mut);
		condcount[cond] += amount;
		size += amount;
		unsigned uboundf = unsigned(double(ubound) * estimatefactor);
		if (size > uboundf) {
			unsigned newsize = 0;
			unsigned k = 0;
			while (k < condcount.size() && newsize+condcount[k] <= uboundf)
			{
				newsize += condcount[k];
				++k;
			}
			for (unsigned j = k+1; j <= maxcond && j < condcount.size(); ++j)
				condcount[j] = 0;
			if (newsize != 0) {
				size = newsize;
				maxcond = k-1;
				condcount[k] = 0;
			} else {
				size = newsize + condcount[k];
				maxcond = k;
			}
		}
	}

	void finish_estimate()
	{
		estimatefactor = 0;
		size = 0;
		count = 0;
		count_balanced = 0;
		if (includenaf)
			if (halfnafweight)
				maxcond += (nafestweight>>1);
			else
				maxcond += nafestweight;
		pathsout.clear();
		pathsout.resize(maxcond + 1);
		condcount.clear();
		condcount.resize(maxcond + 1);
	}

	void push_back(const differentialpath& path, unsigned cond = 0)
	{ 
		if (cond == 0)
		{
			for (int k = int(t)-2; k <= min(tend,path.tend()-1) && k<=64; ++k)
				cond += path[k].hw();
			if (includenaf)
				if (halfnafweight)
					cond += (path[int(t)-3].hw()>>1);
				else
					cond += path[int(t)-3].hw();
		}
		if (cond > maxcond) return;
		if (maxQ26upcond > 0) {
			unsigned cond2 = 0;
			for (int k = int(t)-2; k < path.tend() && k<=64; ++k)
				if (k >= 26)
					cond2 += path[k].hw();
			if (cond2 > maxQ26upcond)
				return;
		}

		if (!noverify)
		{
			if (!test_path_fast(path, m_diff))
			{
				mut.lock();
				++verified;
				++verifiedbad;
				mut.unlock();
				return;
			}
		}
		mut.lock();
		if (!noverify) ++verified;
        if (ubound == 0) {
                pathsout[cond].push_back(path);
				mut.unlock();
                return;
        }
        if (pathsout[cond].size() < ubound) {
                pathsout[cond].push_back(path); 

                ++size, ++count;
                autobalance();
        }
		mut.unlock();
	}

	void autobalance()
	{
		if (size > ubound) {
			++count_balanced;
			unsigned newsize = 0;
			unsigned k = 0;
			while (k < pathsout.size() && k <= maxcond && newsize+pathsout[k].size() <= ubound)
			{
				newsize += unsigned(pathsout[k].size());
				++k;
			}
			if (newsize != 0) {
				size = newsize;
				maxcond = k-1;
			} else {
				size = newsize + pathsout[k].size();
				maxcond = k;
			}
			for (unsigned j = maxcond+2; j < pathsout.size(); ++j)
				pathsout[j].clear();
		}
	}

	void export_results(vector< differentialpath >& outpaths) const
	{
		outpaths.clear();
		outpaths.reserve(ubound);
		for (unsigned k = 0; k < pathsout.size() && k <= maxcond; ++k)
		{
			unsigned index = unsigned(outpaths.size());
			outpaths.resize(index + pathsout[k].size());
			std::copy(pathsout[k].begin(), pathsout[k].end(), outpaths.begin()+index);
		}
		unsigned hbound = unsigned(double(ubound)*fillfraction);
		unsigned k = maxcond+1;
		while (outpaths.size() < hbound && k < pathsout.size()) {
			unsigned index = unsigned(outpaths.size());
			unsigned length = hbound-outpaths.size();
			if (length > pathsout[k].size())
				length = pathsout[k].size();
			outpaths.resize(index + length);
			std::copy(pathsout[k].begin(), pathsout[k].begin()+length, outpaths.begin()+index);
			++k;
		}
	}

	uint32 m_diff[16];

	unsigned t, trange;
	int tend;
	unsigned maxcond;
	unsigned maxsdrs;
	unsigned maxweight, minweight;
	unsigned maxQ26upcond;
	
	bool includenaf;
	unsigned nafestweight;
	bool halfnafweight;
	
	bool noverify;
	unsigned verified, verifiedbad;

	unsigned modn;
	unsigned modi;
	std::string inputfile;
	bool showinputpaths;
	bool newinputpath;

	double estimatefactor;
	unsigned ubound;
	unsigned size;
	unsigned count, count_balanced;
	vector< unsigned > condcount;
	vector< vector< differentialpath > > pathsout;

	double fillfraction;

	int threads;
};



#endif // MAIN_HPP
