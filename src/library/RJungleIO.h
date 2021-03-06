#ifndef RJUNGLEIO_H_
#define RJUNGLEIO_H_

/*
 * Includes
 */

#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include <cstring>

#include "ErrorCodes.h"
#include "config.h"
#include "treedefs.h"
#include "RJunglePar.h"
#include "gzstream.h"
#include "Exception.h"

/*
 * Def.
 */
class RJungleIO {
public:
  RJungleIO() {
    outLog = NULL;
    outSummary = NULL;
    outPrediction = NULL;
    outConfusion = NULL;
    outConfusion2 = NULL;
    outImportance = NULL;
    outImportance2 = NULL;
    outVarProximity = NULL;
    outSamProximity = NULL;
    outImputedData = NULL;
    outXmlJungle = NULL;
    outTuneMtry = NULL;
    outOutlier = NULL;
    outVotes = NULL;
    outOOB = NULL;
    outPrototypes = NULL;
    inXMLjungle = NULL;
    outExtractData = NULL;
    outYaimp = NULL;
    fileVerbose = NULL;
#ifdef HAVE_TIMEPROF
		outTimeProf = NULL;
#endif
  }

  RJungleIO(const RJungleIO &io) {
    outLog = io.outLog;
    outSummary = io.outSummary;
    outPrediction = io.outPrediction;
    outConfusion = io.outConfusion;
    outConfusion2 = io.outConfusion2;
    outImportance = io.outImportance;
    outImportance2 = io.outImportance2;
    outVarProximity = io.outVarProximity;
    outSamProximity = io.outSamProximity;
    outImputedData = io.outImputedData;
    outXmlJungle = io.outXmlJungle;
    outTuneMtry = io.outTuneMtry;
    outOutlier = io.outOutlier;
    outPrototypes = io.outPrototypes;
    inXMLjungle = io.inXMLjungle;
    outVerbose = io.outVerbose;
    outExtractData = io.outExtractData;
    outYaimp = io.outYaimp;
    fileVerbose = io.fileVerbose;
#ifdef HAVE_TIMEPROF
		outTimeProf = io.outTimeProf;
#endif
  }

  RJungleIO(RJunglePar &par) {
    outLog = NULL;
    outSummary = NULL;
    outPrediction = NULL;
    outConfusion = NULL;
    outConfusion2 = NULL;
    outImportance = NULL;
    outImportance2 = NULL;
    outVarProximity = NULL;
    outSamProximity = NULL;
    outImputedData = NULL;
    outXmlJungle = NULL;
    outTuneMtry = NULL;
    outOutlier = NULL;
    outPrototypes = NULL;
    outVotes = NULL;
    outOOB = NULL;
    inXMLjungle = NULL;
    outExtractData = NULL;
    outYaimp = NULL;
    fileVerbose = NULL;
#ifdef HAVE_TIMEPROF
		outTimeProf = NULL;
#endif

    open(par);
  }

  virtual ~RJungleIO() {
  }

  void close() {
    if (outSummary != NULL) {
      outSummary->close();
      delete outSummary;
    }

    if (fileVerbose != NULL) {
      fileVerbose->close();
      delete fileVerbose;
    }

    if (outXmlJungle != NULL) {
      outXmlJungle->close();
      delete outXmlJungle;
    }

    if (outTuneMtry != NULL) {
      outTuneMtry->close();
      delete outTuneMtry;
    }

    if (outOutlier != NULL) {
      outOutlier->close();
      delete outOutlier;
    }

    if (outVotes != NULL) {
      outVotes->close();
      delete outVotes;
    }

    if (outOOB != NULL) {
      outOOB->close();
      delete outOOB;
    }

    if (inXMLjungle != NULL) {
      inXMLjungle->close();
      delete inXMLjungle;
    }

    if (outVarProximity != NULL) {
      outVarProximity->close();
      delete outVarProximity;
    }

    if (outSamProximity != NULL) {
      outSamProximity->close();
      delete outSamProximity;
    }

    if (outImputedData != NULL) {
      outImputedData->close();
      delete outImputedData;
    }

    if (outLog != NULL) {
      outLog->close();
      delete outLog;
    }

    if (outPrediction != NULL) {
      outPrediction->close();
      delete outPrediction;
    }

    if (outConfusion != NULL) {
      outConfusion->close();
      delete outConfusion;
    }

    if (outConfusion2 != NULL) {
      outConfusion2->close();
      delete outConfusion2;
    }

    if (outExtractData != NULL) {
      outExtractData->close();
      delete outExtractData;
    }

    if (outYaimp != NULL) {
      outYaimp->close();
      delete outYaimp;
    }

    if (outPrototypes != NULL) {
      outPrototypes->close();
      delete outPrototypes;
    }

    if (outImportance != NULL) {
      outImportance->close();
      delete outImportance;
    }

    if (outImportance2 != NULL) {
      outImportance2->close();
      delete outImportance2;
    }

#ifdef HAVE_TIMEPROF
    if (outTimeProf != NULL) {
      outTimeProf->close();
      delete outTimeProf;
    }
#endif

    outLog = NULL;
    outSummary = NULL;
    outPrediction = NULL;
    outConfusion = NULL;
    outConfusion2 = NULL;
    outImportance = NULL;
    outImportance2 = NULL;
    outVerbose = NULL;
    outVarProximity = NULL;
    outSamProximity = NULL;
    outImputedData = NULL;
    outXmlJungle = NULL;
    outTuneMtry = NULL;
    outOutlier = NULL;
    outVotes = NULL;
    outOOB = NULL;
    outPrototypes = NULL;
    inXMLjungle = NULL;
    outExtractData = NULL;
    outYaimp = NULL;
#ifdef HAVE_TIMEPROF
		outTuneMtry = NULL;
#endif
  }

  void open(RJunglePar &par) {
    // open file streams

    outLog = new std::ofstream();
    outLog->open((std::string(par.outprefix).append(".log")).c_str());
    if (!outLog->good())
      throw Exception(ERRORCODE_21);

    if (par.summary_flag) {
      outSummary = new std::ofstream();
      outSummary->open((std::string(par.outprefix).append(".summary")).c_str());
      if (!outSummary->good())
        throw Exception(ERRORCODE_21);
    }

    if (strcmp(par.predict, "") != 0) {
      outPrediction = new std::ofstream();
      outPrediction->open(
        (std::string(par.outprefix).append(".prediction")).c_str());
      if (!outPrediction->good())
        throw Exception(ERRORCODE_21);
    }

    outConfusion = new std::ofstream();
    outConfusion->open(
      (std::string(par.outprefix).append(".confusion")).c_str());
    if (!outConfusion->good())
      throw Exception(ERRORCODE_21);

    if (isClassTree(par)) {
      outConfusion2 = new std::ofstream();
      outConfusion2->open(
        (std::string(par.outprefix).append(".confusion2")).c_str());
      if (!outConfusion2->good())
        throw Exception(ERRORCODE_21);
    }

    outImportance = new std::ofstream();
    outImportance->open(
      (std::string(par.outprefix).append(".importance")).c_str());
    if (!outImportance->good())
      throw Exception(ERRORCODE_21);

    if (par.impMeasure > 1) {
      outImportance2 = new std::ofstream();
      outImportance2->open(
        (std::string(par.outprefix).append(".importance2")).c_str());
      if (!outImportance2->good())
        throw Exception(ERRORCODE_21);
    }

    if (!par.verbose_flag) {
      fileVerbose = new std::ofstream();
      fileVerbose->open((std::string(par.outprefix).append(".verbose")).c_str());
      if (!fileVerbose->good())
        throw Exception(ERRORCODE_21);
    }

    if (par.varproximities > 0) {
      outVarProximity = new std::ofstream();
      outVarProximity->open(
        (std::string(par.outprefix).append(".varproximity")).c_str());
      if (!outVarProximity->good())
        throw Exception(ERRORCODE_21);
    }

    if (par.sampleproximities_flag) {
      //      outSamProximity = new ogzstream();
      outSamProximity = new std::ofstream();
      //#ifdef HAVE_LIBZ
      //      outSamProximity->open((std::string(par.outprefix).append(".samproximity.gz")).c_str());
      //#else
      outSamProximity->open(
        (std::string(par.outprefix).append(".samproximity")).c_str());
      //#endif
      if (!outSamProximity->good())
        throw Exception(ERRORCODE_21);
    }

    if (par.tunemtry > 0) {
      outTuneMtry = new std::ofstream();
      outTuneMtry->open(
        (std::string(par.outprefix).append(".tunemtry")).c_str());
      if (!outTuneMtry->good())
        throw Exception(ERRORCODE_21);
    }

    if (isClassTree(par) && (par.outlier > 0)) {
      outOutlier = new std::ofstream();
      outOutlier->open((std::string(par.outprefix).append(".outlier")).c_str());
      if (!outOutlier->good())
        throw Exception(ERRORCODE_21);
    }

    if (isClassTree(par) && par.votes_flag) {
      outVotes = new std::ofstream();
      outVotes->open((std::string(par.outprefix).append(".votes")).c_str());
      if (!outVotes->good())
        throw Exception(ERRORCODE_21);
    }

    if (par.oob_flag) {
      outOOB = new std::ofstream();
      outOOB->open((std::string(par.outprefix).append(".oob")).c_str());
      if (!outOOB->good())
        throw Exception(ERRORCODE_21);
    }

    if (isClassTree(par) && (par.prototypes > 0)) {
      outPrototypes = new std::ofstream();
      outPrototypes->open(
        (std::string(par.outprefix).append(".prototypes")).c_str());
      if (!outPrototypes->good())
        throw Exception(ERRORCODE_21);
    }

    if (par.saveJungleType > 0) {
      outXmlJungle = new std::ofstream();
      //      outXmlJungle = new ogzstream();
      //#ifdef HAVE_LIBZ
      //      outXmlJungle->open((std::string(par.outprefix).append(".jungle.xml.gz")).c_str());
      //#else
      outXmlJungle->open(
        (std::string(par.outprefix).append(".jungle.xml")).c_str());
      //#endif

      if (!outXmlJungle->good())
        throw Exception(ERRORCODE_21);
    }

    if (par.imputeIt > 0) {
      outImputedData = new std::ofstream();
      //      outImputedData = new ogzstream();
      //#ifdef HAVE_LIBZ
      //      outImputedData->open(
      //          (std::string(par.outprefix).append(".imputed.dat.gz")).c_str());
      //#else
      outImputedData->open(
        (std::string(par.outprefix).append(".imputed.dat")).c_str());
      //#endif
      if (!outImputedData->good())
        throw Exception(ERRORCODE_21);
    }

    if (par.yaimp_flag) {
      outYaimp = new std::ofstream();
      outYaimp->open(
        (std::string(par.outprefix).append(".yaimportance")).c_str());
      if (!outYaimp->good())
        throw Exception(ERRORCODE_21);
    }

    if (par.extractdata_flag) {
      outExtractData = new ogzstream();
#ifdef HAVE_LIBZ
      outExtractData->open((std::string(par.outprefix).append(
            ".extracted.dat.gz")).c_str());
#else
      outExtractData->open(
        (std::string(par.outprefix).append(".extracted.dat")).c_str());
#endif
      if (!outExtractData->good())
        throw Exception(ERRORCODE_21);
    }

#ifdef HAVE_TIMEPROF
		outTimeProf = new std::ofstream();
		outTimeProf->open(
				(std::string(par.outprefix).append(".timeprof")).c_str());
		if (!outTimeProf->good())
			throw Exception(ERRORCODE_21);
#endif

    // output files
    if (par.verbose_flag) {
      outVerbose = &std::cout;
    } else {
      outVerbose = fileVerbose;
    }
  }

  bool isClassTree(RJunglePar &par) {
    return (par.treeType == tt_CART) || (par.treeType == tt_CARTtwoing)
      || (par.treeType == tt_CARTsrt) || (par.treeType == tt_CARTcor)
      || (par.treeType == tt_CICase1) || (par.treeType == tt_CICase3)
      || (par.treeType == tt_CISNP);
  }

  std::ofstream *outLog;
  std::ofstream *outSummary;
  std::ofstream *outPrediction;
  std::ofstream *outConfusion;
  std::ofstream *outConfusion2;
  std::ofstream *outImportance;
  std::ofstream *outImportance2;
  std::ofstream *fileVerbose;
  std::ofstream *outVarProximity;
  //  ogzstream *outSamProximity;
  std::ofstream *outSamProximity;
  //ogzstream *outImputedData;
  std::ofstream *outImputedData;
  std::ofstream *outXmlJungle;
  std::ofstream *outTuneMtry;
  std::ofstream *outOutlier;
  std::ofstream *outVotes;
  std::ofstream *outOOB;
  std::ofstream *outPrototypes;
  igzstream *inXMLjungle;
  ogzstream *outExtractData;
  std::ofstream *outYaimp;
  std::ostream *outVerbose;
#ifdef HAVE_TIMEPROF
  std::ofstream *outTimeProf;
#endif

};

#endif /* RJUNGLEIO_H_ */
