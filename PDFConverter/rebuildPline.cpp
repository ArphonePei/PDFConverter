#include "StdAfx.h"
#include "rebuildPline.h"
#include <algorithm>
#include <math.h>

double UserHelp::m_cPI2 = 8.0 * atan(1.0);

UserHelp::UserHelp(void)
{
}

UserHelp::~UserHelp(void)
{
}

AcDbObjectId UserHelp::Append(AcDbEntity* pEnt)
{
	AcDbBlockTable *pBlockTable;
	acdbHostApplicationServices()->workingDatabase()->getSymbolTable(pBlockTable, AcDb::kForRead);

	AcDbBlockTableRecord *pBlockTableRecord;
	pBlockTable->getAt(ACDB_MODEL_SPACE, pBlockTableRecord,
		AcDb::kForWrite);
	pBlockTable->close();

	AcDbObjectId EntId;
	pBlockTableRecord->appendAcDbEntity(EntId, pEnt);

	pBlockTableRecord->close();
	return EntId;
}

double UserHelp::GetBulge(AcDbArc*& pArc)
{
	double dStartAngle = pArc->startAngle();
	double dEndAngle = pArc->endAngle();

	double dAlfa = dEndAngle - dStartAngle;
	if (dAlfa < 0.0)//如果终点角度小于起点角度，取补角
	{
		dAlfa = m_cPI2 + dAlfa;
	}

	double dBulge = 0.0;

	dBulge = tan((dAlfa) / 4.0);
	return dBulge;
}


bool UserHelp::FindNextPointNum(vector<NumMap>& nm, int nThisNum, double& dThisBulge, int& nNextNum)
{
	bool bFind = false;

	//以第一个编号给为起点查找
	vector<NumMap>::iterator itr;
	for (itr = nm.begin(); itr != nm.end(); itr++)
	{
		if ((*itr).nNum1 == nThisNum)
		{
			nNextNum = (*itr).nNum2;
			dThisBulge = (*itr).dBulge;
			bFind = true;
			nm.erase(itr);
			break;
		}
	}

	if (!bFind)//如果以第一个编号给为起点未找到,就以第二个编号给为起点查找
	{
		for (itr = nm.begin(); itr != nm.end(); itr++)
		{
			if ((*itr).nNum2 == nThisNum)
			{
				nNextNum = (*itr).nNum1;
				dThisBulge = -(*itr).dBulge;//凸度取反
				bFind = true;
				nm.erase(itr);
				break;
			}
		}
	} 

	if (bFind)
	{
		return true;
	}
	else
	{
		nNextNum = -1;
		dThisBulge = 0.0;
	}

	return false;
}

bool UserHelp::NumToPoint(vector<PointNum>& vecPt2Num, int nThisNum, AcGePoint3d& ptThis)
{
	bool bFind = false;
	vector<PointNum>::iterator itr;
	for (itr = vecPt2Num.begin(); itr != vecPt2Num.end(); itr++)
	{
		if ((*itr).nNum == nThisNum)
		{
			ptThis = (*itr).pt;
			bFind = true;
			break;
		}
	}

	return bFind;
}

void UserHelp::GetDrawInfo(vector<PointNum>& vecPt2Num, vector<NumMap>& vecNumMap, int& nCodeNum,
						   const AcGePoint3d& ptStart, const AcGePoint3d ptEnd, double dBulge)
{
	bool bStartNew = true; //起点是否编号
	bool bEndNew = true; //终点是否编号

	NumMap nummap;//两点之间的联系信息
	nummap.dBulge = dBulge;

	//处理起点
	vector<PointNum>::iterator itr;
	for (itr = vecPt2Num.begin(); itr != vecPt2Num.end(); itr++)
	{
		if ((*itr).pt == ptStart)
		{
			bStartNew = false; 
			nummap.nNum1 = (*itr).nNum;
			break;
		}
	}

	if (bStartNew)
	{
		PointNum pn;
		pn.nNum = nCodeNum++;
		pn.pt = ptStart;
		vecPt2Num.push_back(pn);

		nummap.nNum1 = pn.nNum;
	}

	//处理终点
	for (itr = vecPt2Num.begin(); itr != vecPt2Num.end(); itr++)
	{
		if ((*itr).pt == ptEnd)
		{
			bEndNew = false;
			nummap.nNum2 = (*itr).nNum;
			break;
		}
	}

	if (bEndNew)
	{
		PointNum pn;
		pn.nNum = nCodeNum++;
		pn.pt = ptEnd;
		vecPt2Num.push_back(pn);
		nummap.nNum2 = pn.nNum;
	}

	//添加联系信息
	vecNumMap.push_back(nummap);
}

bool compWidth(AcDbPolyline* pl1, AcDbPolyline* pl2)
{
	double w1 = -1;
	double w2 = -1;
	pl1->getConstantWidth(w1);
	pl2->getConstantWidth(w2);
	return (w1 < w2);
}

void UserHelp::AppendPlines(std::vector<AcDbPolyline*>& arrPtr, AcDbDatabase* pDb)
{
	if (arrPtr.size()<=0)
	{
		return;
	}
	std::sort(arrPtr.begin(), arrPtr.end(), compWidth);
	double curWid = -1;
	arrPtr[0]->getConstantWidth(curWid);
	std::vector<AcDbPolyline*>::iterator cBegItr = arrPtr.begin();
	std::vector<AcDbPolyline*>::iterator cEndItr = cBegItr;
	while ((cEndItr+1) != arrPtr.end())
	{
		double wd = -1;
		(*(cEndItr+1))->getConstantWidth(wd);
		if (abs(curWid-wd) < ZEROFUZZ)
		{
			cEndItr++;
		}
		else
		{
			curWid = wd;
			std::vector<AcDbPolyline*> subArr;
			subArr.assign(cBegItr, cEndItr);
			ChangeToPolyLine(subArr, pDb);
			cEndItr++;
			cBegItr = cEndItr;
		}
	}
	if (cBegItr != arrPtr.end())
	{
		std::vector<AcDbPolyline*> subArr;
		subArr.assign(cBegItr, cEndItr+1);
		ChangeToPolyLine(subArr, pDb);
	}
	for (int i=0; i<arrPtr.size(); i++)
	{
		arrPtr[i]->erase();
		if (eOk != arrPtr[i]->close())
		{
			delete arrPtr[i];
			arrPtr[i] = NULL;
		}
	}
}

class vtxPos
{
public:
	vtxPos() {x=0; y=0;}
	vtxPos(AcGePoint2d& pt)
	{
		x = pt.x;
		y = pt.y;
	}

	bool operator< (const vtxPos& pos) const
	{
		if (x < pos.x)
		{
			return true;
		}
		else if (abs(x - pos.x) < ZEROFUZZ)
		{
			return y < pos.y;
		}
		else
		{
			return false;
		}
	}

	bool operator== (const vtxPos& pos) const
	{
		return ((abs(x-pos.x)<ZEROFUZZ) && (abs(y-pos.y)<ZEROFUZZ));
	}

	void operator= (const AcGePoint2d& pt)
	{
		x = pt.x;
		y = pt.y;
	}

	double x;
	double y;
};

void add2Path(AcGePoint2d& pt, int idx, map<vtxPos, vector<int>>& mapVtx)
{
	vtxPos pos(pt);
	map<vtxPos, vector<int>>::iterator itr = mapVtx.find(pos);
	if (itr == mapVtx.end())
	{
		vector<int> newVtxs;
		newVtxs.push_back(idx);
		mapVtx.insert(pair<vtxPos, vector<int>>(pos, newVtxs));
	}
	else
	{
		itr->second.push_back(idx);
	}
}

class plinePos
{
public:
	plinePos() {}
	plinePos(AcGePoint2d& p1, AcGePoint2d& p2)
	{
		sPt = p1;
		ePt = p2;
	}
	vtxPos sPt;
	vtxPos ePt;
};

class pathPos
{
public:
	pathPos(vtxPos& p1, vtxPos& p2, int idx)
	{
		paths.push_back(p1);
		paths.push_back(p2);
		index.push_back(idx);
	}
	vector<vtxPos> paths;
	vector<int> index;
};

bool expandPath(pathPos& path, map<vtxPos, vector<int>>& mapVtx, vector<plinePos>& vecPlines, bool bFront)
{
	bool bFlag = false;

	vtxPos curPt = bFront ? (path.paths[0]) : (path.paths[path.paths.size()-1]);
	map<vtxPos, vector<int>>::iterator mapItr = mapVtx.find(curPt);
	if (mapItr == mapVtx.end())
	{
		ASSERT(FALSE);
		return false;
	}
	int curIdx = bFront ? (path.index[0]) : (path.index[path.index.size()-1]);
	vector<int>::iterator vecItr = std::find(mapItr->second.begin(), mapItr->second.end(), curIdx);
	if (vecItr != mapItr->second.end())
	{
		mapItr->second.erase(vecItr);
		if (mapItr->second.size()>0)
		{
			bFlag = true;
			int nextIdx = mapItr->second[0];
			vtxPos nextPos;
			if (vecPlines[nextIdx].sPt == mapItr->first)
			{
				nextPos = vecPlines[nextIdx].ePt;
			}
			else
			{
				nextPos = vecPlines[nextIdx].sPt;
			}
			if (bFront)
			{
				path.index.insert(path.index.begin(), nextIdx);
				path.paths.insert(path.paths.begin(), nextPos);
			}
			else
			{
				path.index.push_back(nextIdx);
				path.paths.push_back(nextPos);
			}
			mapItr->second.erase(mapItr->second.begin());
		}

		if (bFlag)
		{
			expandPath(path, mapVtx, vecPlines, bFront);
		}
		
		return true;
	}
	else
	{
		return false;
	}
}

bool UserHelp::ChangeToPolyLine(std::vector<AcDbPolyline*>& arrPtr, AcDbDatabase* pDb)
{
	Zcad::ErrorStatus es = eOk;

	int length = arrPtr.size();
	map<vtxPos, vector<int>> mapVtx;
	vector<plinePos> vecPlines;
	for (int i = 0; i < length; i++) 
	{
		if (!arrPtr[i])
		{
			continue; 
		}
		AcDbPolyline* pPolyLine = arrPtr[i];
		if (pPolyLine->numVerts() != 2)
		{
			continue; 
		}
		AcGePoint2d sPt, ePt;
		pPolyLine->getPointAt(0, sPt);
		pPolyLine->getPointAt(1, ePt);
		plinePos pline(sPt, ePt);
		vecPlines.push_back(pline);
		add2Path(sPt, vecPlines.size()-1, mapVtx);
		add2Path(ePt, vecPlines.size()-1, mapVtx);
	}

	vector<pathPos> vecPath;
	for (int i=0; i < vecPlines.size(); i++)
	{
		pathPos newPath(vecPlines[i].sPt, vecPlines[i].ePt, i);
		if ((vecPlines[i].sPt == vecPlines[i].ePt) || 
			(expandPath(newPath, mapVtx, vecPlines, true) && expandPath(newPath, mapVtx, vecPlines, false)))
		{
			vecPath.push_back(newPath);
		}
	}

	//绘制多段线
	for (int i=0; i<vecPath.size(); i++)
	{
		AcDbPolyline* pPline = new AcDbPolyline;
		for (int j=0; j<vecPath[i].paths.size(); j++)
		{
			pPline->addVertexAt(j, AcGePoint2d(vecPath[i].paths[j].x, vecPath[i].paths[j].y), 0);
		}
		if (vecPath[i].paths[0] == vecPath[i].paths[vecPath[i].paths.size()-1])
		{
			pPline->setClosed(Adesk::kTrue);
		}
		pPline->setPropertiesFrom(arrPtr[0]);
		double wd = -1;
		arrPtr[0]->getConstantWidth(wd);
		pPline->setConstantWidth(wd);
		arrPtr[0]->handOverTo(pPline);
		pPline->close();
		arrPtr.erase(arrPtr.begin());
	}
	return true;
}
