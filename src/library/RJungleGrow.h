/*
 * RJungleGrow.h
 *
 *  Created on: 06.11.2008
 *      Author: schwarz
 */

#ifndef RJUNGLEGROW_H_
#define RJUNGLEGROW_H_

/*
 * Includes
 */

#include <iostream>
#include <vector>
#include <ctime>
#ifdef _OPENMP
#include <omp.h>
#endif

#include "RJunglePar.h"
#include "RJungleIO.h"
#include "RJungleGen.h"
#include "RJungleProxi.h"
#include "RJungleHelper.h"
#include "RJungleCompiler.h"
#include "RJungleConfusion.h"
#include "RJungleImportance.h"
#include "DataFrame.h"
#include "DataTreeSet.h"
#include "Prediction.h"
#include "PermImportance.h"
#include "Proximities.h"
#include "Helper.h"
#include "treedefs.h"
#include "JTreeCtrl.h"
#include "TimeProf.h"

template<class T>
class RJungleGrow {
	public:
		RJungleGrow() {
		}
		;
		virtual ~RJungleGrow() {
		}
		;

		/**
		 * Grow many trees and process various additional methods for analysing data.
		 * @param data RJungle input data.
		 * @param binder A binder which contains all relevant informations for
		 * forest growing.
		 * @param colMaskVec A selection of all available variables.
		 * @param iteration Number of passed iteration in backelimination / imputing
		 * method.
		 */
		static void growLocal(DataFrame<T> &data, RJungleBinder<T> &binder,
				std::vector<uli_t> *&colMaskVec, int iteration) {

			// todo: replace refs in code and erase these decls.
			RJunglePar &par = binder.par;
			RJungleIO &io = binder.io;
			RJungleGen<T> &gen = binder.gen;
			DataTreeSet &oobSet = binder.oobSet;
			//std::vector<CmpldTree<T> *> &cmpldTrees = binder.cmpldTrees;
			Importance<T> *&intrinsicImportance = binder.intrinsicImportance;
			PermImportance<T> *&permImportance = binder.permImportance;
			Prediction<T> &pred = binder.pred;
			Proximities<T> &proxi = binder.proxi;
			Proximities<T> &varProxi = binder.varProxi;

			// def. / decl.
			int k, totalTrees, timeSpan, timeShift;
			clock_t start, end;
			double timeEst;
			double percentRatio = 0.1;
			uli_t percentRate = 0;
			uli_t percent = 0;

			pred.init(&par, &io, &gen, &data);
			timeSpan = timeShift = 20;
			start = end = 0;
			totalTrees = k = 0;
			timeEst = 0;

			std::vector<double> oobpair;

			// variable proximities
			if (doVarProxi(par)) {
				RJungleProxi<T>::initVarProx(data, varProxi);
			}

			// sample proximities
			if (doSampleProxi(par)) {
				oobpair.resize(par.nrow * (par.nrow - 1), 0);
				binder.proxi.initWithData(&data);
			}

			// for fetching forest, a XML reader is needed
			RJungleFromXML<T> xmlReader(par);

			// mpi master does not grow any tree
			//if ((par.mpi == 0) || ((par.mpi == 1) && (par.mpiId != 0))) {
			if (doFetchJungleFromFile(par)) { // fetch forest from XML file

#ifdef __DEBUG__
				std::cout << "Fetch parameters from file"  << std::endl;
#endif
				// prepare oob data
				//binder.oobSet.init(binder.par.nrow, binder.par.ntree);
				//binder.oobSet.setAll();

				// start XML reader
				xmlReader.startXmlReader(&data);

				// fetch parameteres from XML file
				RJunglePar parRestore = xmlReader.getPar();

				data.par.treeType = parRestore.treeType;
				data.par.ntree = parRestore.ntree;

				par.treeType = parRestore.treeType;
				par.ntree = parRestore.ntree;

				binder.par.treeType = parRestore.treeType;
				binder.par.ntree = parRestore.ntree;

				binder.oobSet.init(binder.par.nrow, binder.par.ntree);
				binder.oobSet.setAll();
			}

			// save header of forest file
			if (doSaveJungle(par))
				RJungleHelper<T>::printXmlHeader(par, *io.outXmlJungle);

			// start multiple threads
#pragma omp parallel
			{
				RJunglePar par_omp(par);
				Tree<T, uli_t> *onetree_omp = NULL;
				CmpldTree<T> *oneCmpldtree_omp = NULL;
				gsl_rng *rng_omp;
				double numOfThreads_omp;
				int i_omp, ntree_omp, id_omp, treeId_omp;
				std::vector<uli_t> trainDataRows_omp;
				DataTreeSet oobSet_omp;
				std::vector<uli_t> oobDataRows_omp;
				JTreeCtrl<T> treeCtrl_omp;
				std::vector<uli_t> *colMaskVec_omp = NULL;
				DataFrame<double> *yaimp_omp = NULL;

				trainDataRows_omp.clear();
				oobDataRows_omp.clear();

				uli_t myTreeSize_omp = par_omp.ntree;
#ifdef HAVE_MPI
				if (par_omp.mpiId == 0)
					myTreeSize_omp = par_omp.ntreeMpi;
#endif
				oobSet_omp.init(par_omp.nrow, myTreeSize_omp);

				// if forest will be fetched from file then set oob data
				if (doFetchJungleFromFile(par))
					oobSet_omp = binder.oobSet;

#pragma omp critical
				{
#ifdef _OPENMP
					numOfThreads_omp = (double)omp_get_num_threads();
					id_omp = omp_get_thread_num();
#else
					numOfThreads_omp = 1;
					id_omp = 0;
#endif

					rng_omp = gsl_rng_alloc(gsl_rng_mt19937);
					/*
					 * Since any initial seed except zero lies on the same orbit, the
					 * choice of an initial seed does not affect the randomness for the
					 * whole period.
					 * M MATSUMOTO, T NISHIMURA
					 * "Mersenne Twister: A 623-Dimensionally Equidistributed Uniform
					 *  Pseudo-Random Number Generator"
					 */
#ifdef HAVE_MPI
					gsl_rng_set(rng_omp, (uli_t) (1 + par.mpiSize * numOfThreads_omp * iteration
								+ par.mpiId * numOfThreads_omp + id_omp + par.seed));
#else
					gsl_rng_set(rng_omp, (uli_t) (1 + numOfThreads_omp * iteration
								+ id_omp + par.seed));
#endif
					par_omp.rng = rng_omp;

					// start controller
					treeCtrl_omp = JTreeCtrl<T> (par_omp, io, gen);

					if (colMaskVec != NULL) {
						colMaskVec_omp = new std::vector<uli_t>();
						colMaskVec_omp->assign(colMaskVec->begin(), colMaskVec->end());
					}

					if (par.yaimp_flag) {
						//          yaimp_omp = new DataFrame<double >(par);
						//          yaimp_omp->setDim(par.ncol, par.ncol);
						//          yaimp_omp->initMatrix();
						//          yaimp_omp->setVarNames(data.varNames);
						//          yaimp_omp->setAll(0);
					}
				}

				ntree_omp = (int) floor(par.ntree / numOfThreads_omp);

#pragma omp critical
				if (id_omp != 0)
					totalTrees += ntree_omp;
				//#pragma omp critical
				//if (id_omp != 0) {totalTrees =0; ntree_omp = 0;}

#pragma omp barrier

#pragma omp master
				{
					ntree_omp = (int) par.ntree - totalTrees;
					percentRate = std::max((uli_t) 1, (uli_t) floor(ntree_omp
								* percentRatio));

					*io.outVerbose << numOfThreads_omp << " thread(s) "
						<< (doFetchJungleFromFile(par) ? "processing from file"
								: "growing") << " " << par.ntree << " tree(s)" << std::endl; //<< " (" << ntree_omp;
				}

#pragma omp barrier

				/*
#pragma omp critical
if (id_omp != 0) *io.outVerbose << "+" << ntree_omp;

#pragma omp barrier

#pragma omp master
				 *io.outVerbose << ")" << std::endl;
				 */

				TIMEPROF_START("RJungleCtrl~~RJungleGrow::growLocal::buildalltrees");
				for (i_omp = 0; i_omp < ntree_omp; ++i_omp) {
#pragma omp master
					if ((i_omp < timeSpan + timeShift) && (i_omp >= timeShift))
						start = clock();

#pragma omp critical
					{
						treeId_omp = k;
						k++;
						//std::cout << k << std::endl;
					}

					// read tree from XML file or grow tree
					if (doFetchJungleFromFile(par)) {

#ifdef __DEBUG__
						std::cout << "Fetch tree file"  << std::endl;
#endif
						// fetch tree from file
#pragma omp critical
						oneCmpldtree_omp = xmlReader.getNextTree();

#ifdef __DEBUG__
						std::cout << "treeId_omp" << treeId_omp << std::endl;
						std::cout << "oneCmpldtree_omp" << oneCmpldtree_omp << std::endl;
#endif

						if (oneCmpldtree_omp == NULL) {// could not find tree in file
							Exception(ERRORCODE_56);
							exit(1);
						}
					} else {

						//build tree
						TIMEPROF_START("RJungleGrow~~makeTree");
						onetree_omp = treeCtrl_omp.makeTree(&data, colMaskVec_omp,
								treeId_omp, par.downsampling_flag,
								//OUTPUT
								trainDataRows_omp, oobSet_omp, oobDataRows_omp, yaimp_omp);
						TIMEPROF_STOP("RJungleGrow~~makeTree");

						// compile jungle in a machine friendly format for high speed calc.
						TIMEPROF_START("RJungleGrow~~cmplTree");
						oneCmpldtree_omp = gen.fct.cmplTree(*onetree_omp);
						TIMEPROF_STOP("RJungleGrow~~cmplTree");

						// clean up
						delete onetree_omp;
					}

					/*
						 if (par.yaimp_flag) {
						 yaimp_omp->div((double)par.ntree);

					//#pragma omp critical
					yaimp->add(*yaimp_omp);

					yaimp_omp->pow2();
					yaimp_omp->mult((double)par.ntree);

					//#pragma omp critical
					yaimpVariation->add(*yaimp_omp);

					yaimp_omp->setAll(0);
					}
					*/

					// get prediction of oob set
					TIMEPROF_START("RJungleCtrl~~RJungleConfusion::getAndPrintOobPredictions");
					pred.add(oneCmpldtree_omp, &oobSet_omp, treeId_omp);
					TIMEPROF_STOP("RJungleCtrl~~RJungleConfusion::getAndPrintOobPredictions");

					// merge intrinsicVar
#pragma omp critical
					if (ntree_omp > 0) {

						// set intrinsic importance
						// if tree was read from XML file
						// then no intrinsic importance is available
						if (!doFetchJungleFromFile(par)) {
							intrinsicImportance->setIteration(iteration);
							intrinsicImportance->add(treeCtrl_omp.getVarImp());
						}

						// permutation importance
						TIMEPROF_START("RJungleGrow~~permImp");
						if (doPermImp(par, colMaskVec)) {
							permImportance->setIteration(iteration);
							permImportance->add(oneCmpldtree_omp, oobSet_omp, treeId_omp,
									pred, colMaskVec);
						}
						TIMEPROF_STOP("RJungleGrow~~permImp");

						//          TImportance<T >* impDebug = static_cast<TImportance<T >* >(treeCtrl_omp.getVarImp());
						//          Helper::printVec<double >(impDebug->amounts);

						// sample proximities
						if (doSampleProxi(par))
							RJungleProxi<T>::updateProximitiesCmpld(&data, oneCmpldtree_omp,
									gen.fct, oobSet_omp, oobpair, proxi, treeId_omp);

						// variable proximities
						if (doVarProxi(par))
							RJungleProxi<T>::updateVarProximitiesCmpld(gen, data,
									oneCmpldtree_omp, colMaskVec, varProxi);

						// print jungle summary
						if (par.summary_flag) {
							//*io.outVerbose << "Writing summary to file..." << std::endl;
							RJungleHelper<T>::summary(io, *oneCmpldtree_omp);
						}

						// save tree
						if (doSaveJungle(par)) {
							switch (par.saveJungleType) {
								case 1:
									//RJungleHelper<T>::printXml(io, oneCmpldtree_omp);
									break;
								case 2:
									RJungleHelper<T>::printXmlRaw(io, oneCmpldtree_omp, treeId_omp);
									break;
								default:
									throw Exception(ERRORCODE_46);
							}
						}
					}

					// merge yet another importance matrix
					//        if (par.yaimp_flag) yaimp->add(*yaimp_omp);

#pragma omp critical
					delete oneCmpldtree_omp;

#pragma omp master
					{
						if ((i_omp < timeSpan + timeShift) && (i_omp >= timeShift)) {
							end = clock();
							timeEst += (double) (end - start) / CLOCKS_PER_SEC;
						}

						if ((uli_t) i_omp == percent + percentRate) {
							percent = percent + percentRate;
							*io.outVerbose << "progress: " << (uli_t) (100.0
									* (double) percent / ntree_omp) << "%" << std::endl;
						}

						if (i_omp == (timeSpan - 1 + timeShift)) {
							timeEst = (par.ntree / numOfThreads_omp - 1) * timeEst / timeSpan
								/ numOfThreads_omp;

							*io.outVerbose << "Growing time estimate: ";
							Helper::printTime(timeEst, *io.outVerbose);

							*io.outVerbose << std::endl;
						}
					}
				}
				TIMEPROF_STOP("RJungleCtrl~~RJungleGrow::growLocal::buildalltrees");

				// clean up
#pragma omp critical
				{
					if (colMaskVec_omp != NULL) {
						delete colMaskVec_omp;
						colMaskVec_omp = NULL;
					}
				}

#pragma omp master
				*io.outVerbose << "Generating and collecting output data..."
					<< std::endl;

				gsl_rng_free(rng_omp);

				/*
				 *  merge variable contents
				 */

				//if (intrinsicVar.size() <= par.numOfImpVar)
				{

					// merge oobSet
#pragma omp critical
					if (!doFetchJungleFromFile(par)) {
#ifdef __DEBUG__
						*io.outVerbose << "oobSet.nrow = " << oobSet.nrow << std::endl;
						*io.outVerbose << "oobSet.ncol = " << oobSet.ncol << std::endl;
						*io.outVerbose << "oobSet_omp.nrow = " << oobSet_omp.nrow << std::endl;
						*io.outVerbose << "oobSet_omp.ncol = " << oobSet_omp.ncol << std::endl;
#endif
						oobSet |= oobSet_omp;
					}

				}

#pragma omp master
				if (par.yaimp_flag) {
					delete yaimp_omp;
					yaimp_omp = NULL;
				}
			}
			//} // at the points slaves return when performing mpi

			// finalize
#ifdef HAVE_MPI
			// combining mpi slave data
			if (par.mpi == 1) {
				// permutation importance
				if (doPermImp(par, colMaskVec)) {
					permImportance->setIteration(iteration);
					permImportance->combineMpi(colMaskVec);
				}

				// oob data
				oobSet.combineMpi(par);

				// prediction data
				pred.combineMpi();

				// sample proxi
				if (doSampleProxi(par))
					proxi.combineMpi();

				// variable proxi
				if (doVarProxi(par))
					varProxi.combineMpi();
			}
#endif

			// permutation importance
#ifdef HAVE_MPI
			if (par.mpiId == 0) {
#endif
				// normalize sample proximities
				if (doSampleProxi(par))
					RJungleProxi<T>::normalizeProximitiesCmpld(&data, oobpair, proxi);

				// get sample proximities
				if (doSampleProxi(par))
					RJungleProxi<T>::finalize(proxi);

				// variable proximities
				if (doVarProxi(par))
					RJungleProxi<T>::finalizeVarProx(io, data, colMaskVec, varProxi);

				if (doPermImp(par, colMaskVec)) {
#ifdef __DEBUG__
					*io.outVerbose << "Finalize permutation importance." << std::endl;
#endif
					TIMEPROF_START("RJungleGrow~~permImp");
					permImportance->finalize();
					TIMEPROF_STOP("RJungleGrow~~permImp");
#ifdef __DEBUG__
					*io.outVerbose << "Save permutation importance." << std::endl;
#endif
					permImportance->save();
				}
#ifdef HAVE_MPI
			}
#endif

			// close saved forest
			if (doSaveJungle(par))
				RJungleHelper<T>::printXmlFooter(io);

			// stop reading xml file
			if (doFetchJungleFromFile(par))
				xmlReader.stopXmlReader();

#ifdef __DEBUG__
			*io.outVerbose << "Write prediction to file." << std::endl;
#endif
			// write prediction to file
			if (doFetchJungleFromFile(par))
				RJunglePrediction<T>::showPredictionCmpld(io, data, pred, oobSet,
						iteration);

		}

		static bool twoWayInteractionCmp(
				std::pair<double, std::pair<uli_t, uli_t> > a, std::pair<double,
				std::pair<uli_t, uli_t> > b) {
			return (a.first > b.first);
		}

		static bool doSampleProxi(RJunglePar &par) {
			bool out = false;

			if (par.sampleproximities_flag || (par.outlier > 0) || (par.prototypes > 0)
					|| (par.imputeIt > 1))
				out = true;

			return out;
		}

		static bool doPermImp(RJunglePar &par, std::vector<uli_t> *colMaskVec) {
			bool out = false;

			if ((((par.backSel == bs_DIAZURIATE)) && (colMaskVec == NULL))
					|| (par.backSel != bs_DIAZURIATE)) {
				if ((par.impMeasure == im_perm_breiman) || (par.impMeasure
							== im_perm_liaw) || (par.impMeasure == im_perm_raw)
						|| (par.impMeasure == im_perm_meng)) {
					out = true;
				}
			}

			return out;
		}

		static bool doVarProxi(RJunglePar &par) {
			bool out = false;

			if (par.varproximities > 0) {
				out = true;
			}

			return out;
		}

		static bool doSaveJungle(RJunglePar &par) {
			return (par.saveJungleType > 0);
		}

		static bool doFetchJungleFromFile(RJunglePar &par) {
			return (strcmp(par.predict, "") != 0);
		}

};

#endif /* RJUNGLEGROW_H_ */
