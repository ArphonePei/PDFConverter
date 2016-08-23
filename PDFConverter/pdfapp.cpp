#include "StdAfx.h"
#include "resource.h"
#include "AcExtensionModule.h"

#include "rxmfcapi.h"

#include "pdfapp.h"
#include "rebuildPline.h"



BOOL REALEQ(double d1, double d2, double fuzz=ZEROFUZZ);
BOOL REALEQ(double d1, double d2, double fuzz)
{
	return fz_abs(d1 - d2) < fuzz;
}

struct TextProp 
{
	AcString str;
	AcGePoint3d pos;
	fz_point orignPos;
	double height, hFactor, wFactor, tracking, orignHeight;
};

void pdfapp_AcString2Utf8Char(AcString& str, char* utf8Name, int len=PATH_MAX);
void pdfapp_AcString2Utf8Char(AcString& str, char* utf8Name, int len)
{
	const wchar_t* wname = str.kwszPtr();
	WideCharToMultiByte(CP_UTF8, 0, wname, -1, utf8Name, len, NULL, NULL);
}

void pdfapp_Utf8Char2AcString(const char* utf8Name, ACHAR* str, int len=PATH_MAX);
void pdfapp_Utf8Char2AcString(const char* utf8Name, ACHAR* str, int len)
{
	MultiByteToWideChar(CP_UTF8, 0, utf8Name, -1, str, len);
}

void removeExt(const ACHAR* fullName, ACHAR* out)
{
	ACHAR fname[_MAX_FNAME];
	ACHAR dir[_MAX_DIR];
	ACHAR drive[_MAX_DRIVE];
	_wsplitpath(fullName, drive, dir, fname, NULL);
	wcscpy_s(out, _MAX_PATH, drive);
	wcscat_s(out, _MAX_PATH, dir);
	wcscat_s(out, _MAX_PATH, fname);
}

void appInit(pdfapp_t* app)
{
	app->doc = 0;
	app->docpath = 0;
	app->doctitle = 0;
	app->outline = 0;
	app->outline_deferred = 0;
	app->pagecount = 0;
	app->resolution = 0;
	app->rotate = 0;
	app->image = 0;
	app->grayscale = 0;
	app->colorspace = 0;
	app->invert = 0;
	app->presentation_mode = 0;
	app->transitions_enabled = 0;
	app->old_image = 0;
	app->new_image = 0;
	app->start_time = 0;
	app->in_transit = 0;
	app->duration = 0;
	memset(&app->transition, 0, sizeof(fz_transition));
	app->pageno = 0;
	app->page = 0;
	memset(&app->page_bbox, 0, sizeof(fz_rect));
	app->page_list = 0;
	app->annotations_list = 0;
	app->page_text = 0;
	app->page_sheet = 0;
	app->page_links = 0;
	app->errored = 0;
	app->incomplete = 0;
	memset(&app->hist, 0, sizeof(int)*256);
	app->histlen = 0;
	memset(&app->marks, 0, sizeof(int)*10);
	app->winw = 0;
	app->winh = 0;
	app->scrw = 0;
	app->scrh = 0;
	app->shrinkwrap = 0;
	app->fullscreen = 0;
	memset(&app->number, 0, sizeof(char)*256);
	app->numberlen = 0;
	app->ispanning = 0;
	app->panx = 0;
	app->pany = 0;

	app->iscopying = 0;
	app->selx = 0;
	app->sely = 0;
	app->beyondy = 0;
	memset(&app->selr, 0, sizeof(fz_rect));
	app->nowaitcursor;
	app->isediting = 0;
	app->searchdir = 0;
	memset(&app->search, 0, sizeof(char)*512);
	app->searchpage = 0;
	memset(&app->search, 0, sizeof(char)*512);
	app->hit_count = 0;
	app->userdata = 0;
	app->ctx = 0;
}

void pdfapp_init(fz_context* ctx, pdfapp_t* app)
{
	//不能用memset, 因为在pdfapp_t里添加了一些类和STL容器
// 	memset(app, 0, sizeof(pdfapp_t));
	appInit(app);
	app->scrw = 640;
	app->scrh = 480;
	app->resolution = 72;
	app->ctx = ctx;
	app->pDb = NULL;
	app->options.init();
#ifdef _WIN32
	app->colorspace = fz_device_bgr(ctx);
#else
	app->colorspace = fz_device_rgb(ctx);
#endif
}

void pdfapp_close(pdfapp_t* app)
{
	fz_drop_display_list(app->ctx, app->page_list);
	app->page_list = NULL;

	fz_drop_display_list(app->ctx, app->annotations_list);
	app->annotations_list = NULL;

	fz_free_text_page(app->ctx, app->page_text);
	app->page_text = NULL;

	fz_free_text_sheet(app->ctx, app->page_sheet);
	app->page_sheet = NULL;

	fz_drop_link(app->ctx, app->page_links);
	app->page_links = NULL;

	fz_free(app->ctx, app->doctitle);
	app->doctitle = NULL;

	fz_free(app->ctx, app->docpath);
	app->docpath = NULL;

	fz_drop_pixmap(app->ctx, app->image);
	app->image = NULL;

	fz_drop_pixmap(app->ctx, app->new_image);
	app->new_image = NULL;

	fz_drop_pixmap(app->ctx, app->old_image);
	app->old_image = NULL;

	fz_free_outline(app->ctx, app->outline);
	app->outline = NULL;

	fz_free_page(app->doc, app->page);
	app->page = NULL;

	fz_close_document(app->doc);
	app->doc = NULL;

#ifdef HAVE_CURL
	fz_close(app->stream);
#endif

	fz_flush_warnings(app->ctx);
}

AcString pdfapp_password()
{
	ACHAR result[1024];
	if (RTNORM == acedGetString(1, _T("\nPlease input the password: "), result))
	{
		return result;
	}
	else
	{
		return _T("");
	}	
}

AcDbObjectId append2Db(pdfapp_t* app, AcDbEntity* pEnt, BOOL bMirror=TRUE);
AcDbObjectId append2Db(pdfapp_t* app, AcDbEntity* pEnt, BOOL bMirror)
{
	if (bMirror)
	{
		pEnt->transformBy(app->mtxMirror);
	}

	AcDbDatabase* pDb = app->pDb;
	AcDbObjectId ret;
	AcDbBlockTable* pBTbl = NULL;
	pDb->getBlockTable(pBTbl, AcDb::kForRead);
	AcDbBlockTableRecord* pMS = NULL;
	pBTbl->getAt(ACDB_MODEL_SPACE, pMS,	AcDb::kForWrite);
	pBTbl->close();
	pMS->appendAcDbEntity(ret, pEnt);
	pMS->close();

	return ret;
}

AcDbObjectId appendImage2Db(pdfapp_t* app, fz_display_node* pNode, AcString& fileName)
{
	AcDbObjectId ret;
	srand(rand());
	AcString szName;
	szName.format(_T("$PIMG%d"), rand());

	AcDbDatabase* pDb = app->pDb;
	AcDbRasterImageDef* pImageDef = new AcDbRasterImageDef();
	Acad::ErrorStatus es = pImageDef->setSourceFileName(fileName);
	if(es != Acad::eOk)
	{
		delete pImageDef;
		return ret;
	}
	es = pImageDef->load();
	ASSERT(es == Acad::eOk);
	AcDbObjectId dictID = AcDbRasterImageDef::imageDictionary(pDb);

	if (dictID==AcDbObjectId::kNull)
	{
		es = AcDbRasterImageDef::createImageDictionary(pDb, dictID);
		if(es!= Acad::eOk)
		{
			delete pImageDef;
			ads_printf(_T("\nCould not create dictionary\n"));
			return ret;
		}
	}
	AcDbDictionary* pDict = NULL;
	es = acdbOpenObject((AcDbObject*&)pDict, dictID, AcDb::kForWrite);
	if(es != Acad::eOk)
	{
		delete pImageDef;
		ads_printf(_T("\nCould not open dictionary\n"));
		return ret;
	}
	BOOL bExist = pDict->has(szName);
	AcDbObjectId objID;
	//由于图片都是临时文件，不可能出现同一个图片被两个块定义引用的情况，如果插入了同一个PDF两次的话也是引用同一个块定义而已
	while (bExist)
	{
		szName.format(_T("$PIMG%d"), rand());
		bExist = pDict->has(szName);
	}
	pDict->setAt(szName, pImageDef, objID);

	// close Dictionary and Definition. 
	pDict->close();
	AcGeVector2d size = pImageDef->size();
	pImageDef->close();

	AcDbRasterImage* pImage = new AcDbRasterImage;
	es = pImage->setImageDefId(objID);
	if (es != Acad::eOk)
	{
		delete pImage;
		return ret;
	}

	ret = append2Db(app, pImage);

	//这个ctm很奇怪的，不完全是个转换矩阵，而是把(0,0)(1,1)的矩阵转换成实际的位置和大小
	fz_point vecLR = {1, 0};
	fz_transform_vector(&vecLR, &pNode->ctm);
	fz_point vecOP = {0, -1};//PDF跟DWG的y轴是相反的，所以要设为-1
	fz_transform_vector(&vecOP, &pNode->ctm);
	fz_point vecOffset = {0, 1}; //由于PDF中原点是在左上角，而DWG是在左下角，所以要有个offset过来
	fz_transform_vector(&vecOffset, &pNode->ctm);
	AcGeVector3d LowerRightVector(vecLR.x, vecLR.y, 0);
	AcGeVector3d OnPlaneVector(vecOP.x, vecOP.y, 0);

	if (pImage->setOrientation(AcGePoint3d(pNode->ctm.e+vecOffset.x, pNode->ctm.f+vecOffset.y, 0), LowerRightVector, OnPlaneVector) !=Adesk::kTrue)
	{
		ads_printf(_T("\nSet Orientation failed."));
		pImage->close();
		return ret;
	}

	pImage->setDisplayOpt(AcDbRasterImage::kShow, Adesk::kTrue);
	pImage->setDisplayOpt(AcDbRasterImage::kTransparent, Adesk::kTrue);

	AcDbObjectPointer<AcDbRasterImageDefReactor> rasterImageDefReactor;

	// new it 
	rasterImageDefReactor.create();

	// Set the entity to be its owner. 
	es = rasterImageDefReactor->setOwnerId(pImage->objectId());

	// if ok 
	if (es == Acad::eOk)
	{
		AcDbObjectId defReactorId;
		// assign the object an objectId
		es = pDb->addAcDbObject(defReactorId, rasterImageDefReactor.object());

		// if ok
		if (es == Acad::eOk)
		{
			// set the image reactor id
			pImage->setReactorId(defReactorId);

			AcDbObjectPointer<AcDbRasterImageDef> rasterImagedef(pImage->imageDefId(), AcDb::kForWrite);

			// if ok
			if (rasterImagedef.openStatus() == Acad::eOk)
			{
				rasterImagedef->addPersistentReactor(defReactorId);
			}
		}
	}

	pImage->transformBy(app->mtxMirror);
	pImage->close();

	return ret;
}

void createImage(pdfapp_t* app, fz_display_node* pNode, int& idx)
{
	fz_pixmap* pixmap = NULL;
	fz_var(pixmap);
	fz_try(app->ctx)
	{
		pixmap = pNode->item.image->get_pixmap(app->ctx, pNode->item.image, pNode->item.image->w, pNode->item.image->h);
		if (pixmap)
		{
			AcString path;
			ACHAR docPath[PATH_MAX];
			pdfapp_Utf8Char2AcString(app->docpath, docPath);
			ACHAR out[_MAX_PATH];
			removeExt(docPath, out);
			path.format(_T("%s_%d_%d.PNG"), out, app->pageno, idx);
			char utf8Name[PATH_MAX];
			pdfapp_AcString2Utf8Char(path, utf8Name);
			fz_write_png(app->ctx, pixmap, utf8Name, 0);
// 			path.format(_T("%s_%d.PNG"), app->strFileName.kszPtr(), idx);//又搞一遍是因为app->docpath存的路径是UTF8的
			appendImage2Db(app, pNode, path);
			idx++;
		}
	}
	fz_always(app->ctx)
	{
		fz_drop_pixmap(app->ctx, pixmap);
	}
	fz_catch(app->ctx)
	{
		fz_rethrow(app->ctx);
	}
}

void getCurPt(fz_path_item* items, int& i, fz_matrix& ctm, AcGePoint2dArray& pts, AcArray<double>& arrBulge)
{
	//当前点的bulge是由其下一个点来决定的(直线还是圆弧)
	if (pts.length() > 0)
	{
		arrBulge.append(0);
	}
	fz_point pt = {items[++i].v, items[++i].v};
	fz_transform_point(&pt, &ctm);
	pts.append(AcGePoint2d(pt.x, pt.y));
}

void pdfapp_stroke_bezier(AcGePoint2dArray& arrFitPts, double flatness, double xa, double ya, double xb, double yb,
							double xc, double yc, double xd, double yd, int depth)
{
	double dmax;
	double xab, yab;
	double xbc, ybc;
	double xcd, ycd;
	double xabc, yabc;
	double xbcd, ybcd;
	double xabcd, yabcd;

	/* termination check */
	dmax = fz_abs(xa - xb);
	dmax = fz_max(dmax, fz_abs(ya - yb));
	dmax = fz_max(dmax, fz_abs(xd - xc));
	dmax = fz_max(dmax, fz_abs(yd - yc));
	if (dmax < flatness || depth >= MAX_DEPTH)
	{
		arrFitPts.append(AcGePoint2d(xd, yd));
		return;
	}

	xab = xa + xb;
	yab = ya + yb;
	xbc = xb + xc;
	ybc = yb + yc;
	xcd = xc + xd;
	ycd = yc + yd;

	xabc = xab + xbc;
	yabc = yab + ybc;
	xbcd = xbc + xcd;
	ybcd = ybc + ycd;

	xabcd = xabc + xbcd;
	yabcd = yabc + ybcd;

	xab *= 0.5f; yab *= 0.5f;
	xbc *= 0.5f; ybc *= 0.5f;
	xcd *= 0.5f; ycd *= 0.5f;

	xabc *= 0.25f; yabc *= 0.25f;
	xbcd *= 0.25f; ybcd *= 0.25f;

	xabcd *= 0.125f; yabcd *= 0.125f;

	pdfapp_stroke_bezier(arrFitPts, flatness, xa, ya, xab, yab, xabc, yabc, xabcd, yabcd, depth + 1);
	pdfapp_stroke_bezier(arrFitPts, flatness, xabcd, yabcd, xbcd, ybcd, xcd, ycd, xd, yd, depth + 1);
}

inline BOOL IsCoLine(AcGePoint2d p1, AcGePoint2d p2, AcGePoint2d p3)
{
	return abs(((p1.x-p3.x)*(p2.y-p3.y) - (p2.x-p3.x)*(p1.y-p3.y))) < COLINEFUZZ;
}

inline BOOL IsCoCircle(AcGeCircArc2d cir, AcGePoint2d pt)
{
	if ((pt == cir.endPoint()))
	{
		return FALSE;
	}
	double dist = pt.distanceTo(cir.center());
	return abs((dist - cir.radius())) < COCIRCLEFUZZ;
}

double calBulge(AcGePoint2d& tp1, AcGePoint2d&tp2, AcGePoint2d& tmpt)
{
	AcGePoint3d p1(tp1.x, tp1.y, 0);
	AcGePoint3d p2(tp2.x, tp2.y, 0);
	AcGePoint3d mpt(tmpt.x, tmpt.y, 0);
	AcGeLineSeg3d seg(p1, p2);
	AcGePoint3d mid = seg.midPoint();
	AcGeVector3d v = (p2-p1).crossProduct(mpt-mid);
	double bulge = mid.distanceTo(mpt)/p1.distanceTo(mid);
	return (v[2]<0)?bulge:(-bulge);
}

void getCurCurve(fz_path_item* items, int& i, fz_matrix& ctm, double flatness, 
				 AcGePoint2dArray& pts, AcArray<double>& arrBulge, pdfapp_t* app)
{
	AcGePoint2dArray arrFitPts;
	for (int j=0; j<3; j++)
	{
		fz_point pt = {items[++i].v, items[++i].v};
		fz_transform_point(&pt, &ctm);
		arrFitPts.append(AcGePoint2d(pt.x, pt.y));
	}
	if (pts.length() == 0)//容错，正常来说不该出现这个情况
	{
		ASSERT(FALSE);
		pts.append(arrFitPts[2]);
		return;
	}

	AcGePoint2dArray arrCrvPts;
	arrCrvPts.append(pts.last());//方便后面计算
	pdfapp_stroke_bezier(arrCrvPts, flatness, pts.last().x, pts.last().y, arrFitPts[0].x, arrFitPts[0].y, 
						 arrFitPts[1].x, arrFitPts[1].y,  arrFitPts[2].x, arrFitPts[2].y, 0);

	if ((arrCrvPts.length() < 4) || !app->options.m_bArcOpt)
	{
		arrCrvPts.removeAt(0);//第一个点是pts的最后一个点，要去掉避免重复
		pts.append(arrCrvPts);
		for (int i=0; i<arrCrvPts.length(); i++)
		{
			arrBulge.append(0);
		}
	}
	else
	{
		int idx = 0;
		for (idx=0; idx<arrCrvPts.length()-2; idx++)
		{
			if (!IsCoLine(arrCrvPts[idx], arrCrvPts[idx+1], arrCrvPts[idx+2]))
			{
				AcGeCircArc2d curCir(arrCrvPts[idx], arrCrvPts[idx+1], arrCrvPts[idx+2]);
				while (((idx+3)<arrCrvPts.length()) && (IsCoCircle(curCir, arrCrvPts[idx+3])))
				{
					curCir.set(curCir.startPoint(), arrCrvPts[idx+2], arrCrvPts[idx+3]);
					idx++;
				}
				idx++;//下一个循环里面的i应该是本次共圆的最后一个点，即当前的idx+2， 而之后会idx++，所以这里idx++一下就可以了
				arrBulge.append(calBulge(pts.last(), curCir.endPoint(), curCir.evalPoint((curCir.paramOf(curCir.endPoint()) - curCir.paramOf(pts.last())) / 2)));
				pts.append(curCir.endPoint());
			}
			else if (arrCrvPts[idx+2] != pts.last())
			{
				pts.append(arrCrvPts[idx+2]);
				arrBulge.append(0);
			}
		}
		idx++;
		if (idx != (arrCrvPts.length()-2))
		{
			for (int j=idx; j<arrCrvPts.length(); j++)
			{
				pts.append(arrCrvPts[j]);
				arrBulge.append(0);
			}
		}
	}
}

void appendPath(AcGePoint2dArray& pts, AcGeDoubleArray& arrBulge, AcDbDatabase* pDb, 
				AcCmColor& color, double width, Adesk::Boolean bIsClosed, fz_display_command cmd, pdfapp_t* app,
				std::map<Adesk::UInt16, std::vector<AcDbPolyline*>>& mapLineSeg, AcDbHatch* pHatch)
{
	if (pts.length() >1)
	{
		arrBulge.append(0);//最后一个点的bulge

		if (cmd == FZ_CMD_FILL_PATH)
		{
			//不知道为什么如果首尾两点不重合的话填充的样子就很奇怪
			pts.append(pts[0]);
			arrBulge.append(arrBulge[0]);
			if (pHatch)
			{
				pHatch->appendLoop(AcDbHatch::kExternal, pts, arrBulge);
			}
			else
			{
				ASSERT(FALSE);
			}
		}
		else
		{
			AcDbPolyline* pPline = new AcDbPolyline;
			for (int i=0; i<pts.length(); i++)
			{
				pPline->addVertexAt(i, pts[i], arrBulge[i]);
			}
			pPline->setColor(color);
			pPline->setConstantWidth(width);
			pPline->setClosed(bIsClosed);
			if (app->options.m_bLineSegOpt && (pts.length() == 2) && (abs(arrBulge[0]) < ZEROFUZZ))
			{
				append2Db(app, pPline);//注意还没close的, 会在之后优化短线段的时候handover出去
				std::map<Adesk::UInt16, std::vector<AcDbPolyline*>>::iterator itr = mapLineSeg.find(color.colorIndex());
				if (itr == mapLineSeg.end())
				{
					std::vector<AcDbPolyline*> plines;
					plines.push_back(pPline);
					mapLineSeg.insert(std::pair<Adesk::UInt16, std::vector<AcDbPolyline*>>(color.colorIndex(), plines));
				}
				else
				{
					itr->second.push_back(pPline);
				}
			}
			else
			{
				append2Db(app, pPline);
				pPline->close();
			}
		}

		pts.removeAll();
		arrBulge.removeAll();
	}
}

AcCmColor getColor(pdfapp_t* app, fz_display_node* pNode)
{
	float rgb[FZ_MAX_COLORS];
	pNode->colorspace->to_rgb(app->ctx, pNode->colorspace, pNode->color, rgb);
	AcCmColor color;
	color.setColorMethod(AcCmEntityColor::ColorMethod::kByColor);
	color.setRGB(rgb[0]*255, rgb[1]*255, rgb[2]*255);
	if ((color.red() <= 50) && (color.green() <= 50) && (color.blue() <= 50))
	{
		//color.setColorIndex(0);
		//color.setColorMethod(AcCmEntityColor::ColorMethod::kByBlock);
		color.setRGB(255-rgb[0]*255, 255-rgb[1]*255, 255-rgb[2]*255);
	}
	return color;
}

void createPath(pdfapp_t* app, fz_display_node* pNode, std::map<Adesk::UInt16, std::vector<AcDbPolyline*>>& mapLineSeg)
{
	double expansion = fz_matrix_expansion(&pNode->ctm);
	double flatness = 0.3f / expansion;//0.3是从mupdf拿过来的magic number

	AcCmColor color = getColor(app, pNode);

	double linewidth = -1;
	AcDbHatch* pHatch = NULL;
	if ((pNode->cmd == FZ_CMD_STROKE_PATH) && pNode->stroke)
	{
		//expansion其实就是转换矩阵中的比例
		linewidth = pNode->stroke->linewidth * expansion;
	}
	else if (pNode->cmd == FZ_CMD_FILL_PATH)//Fill Path会有孤岛的情况，所以要把Fill Path看成是一整个Hatch
	{
		pHatch = new AcDbHatch();
		pHatch->setNormal(AcGeVector3d(0,0,1));
		pHatch->setElevation(0);
		pHatch->setAssociative(false);
		pHatch->setPattern(AcDbHatch::kPreDefined, _T("SOLID"));
		if (pNode->flag)//even_odd
		{
			pHatch->setHatchStyle(AcDbHatch::kNormal);
		}
		else
		{
			pHatch->setHatchStyle(AcDbHatch::kOuter);
		}
	}

	Adesk::Boolean bIsClosed = kFalse;
	AcGePoint2dArray curPts;
	AcGeDoubleArray arrBulge;
	for (int i=0; i<pNode->item.path->len; i++)
	{
		switch (pNode->item.path->items[i].k)
		{
		case FZ_MOVETO:
			appendPath(curPts, arrBulge, app->pDb, color, linewidth, bIsClosed, pNode->cmd, app, mapLineSeg, pHatch);
			getCurPt(pNode->item.path->items, i, pNode->ctm, curPts, arrBulge);
			bIsClosed = kFalse;
			break;
		case FZ_LINETO:
			getCurPt(pNode->item.path->items, i, pNode->ctm, curPts, arrBulge);
			break;
		case FZ_CURVETO:
			getCurCurve(pNode->item.path->items, i, pNode->ctm, flatness, curPts, arrBulge, app);
			break;
		case FZ_CLOSE_PATH:
			bIsClosed = kTrue;
			break;
		}
	}
	appendPath(curPts, arrBulge, app->pDb, color, linewidth, bIsClosed, pNode->cmd, app, mapLineSeg, pHatch);
	if (pHatch)
	{
		pHatch->evaluateHatch();
		pHatch->setColor(color);
		append2Db(app, pHatch);
		pHatch->close();
	}
}

void getBBoxFeature(pdfapp_t* app, fz_display_node* pNode, int i, fz_matrix& mtx, AcGePoint3d& pos, double& boxHeight, double& boxWidth, fz_point& oPt, double& oHeight)
{
	fz_rect bbox;
	fz_bound_glyph(app->ctx, pNode->item.text->font, pNode->item.text->items[i].gid, &fz_identity, &bbox);
	fz_point pt;
	oPt.x = pt.x = bbox.x0;
	oPt.y = pt.y = bbox.y0;
	oHeight = fz_abs(bbox.y1 - bbox.y0);
	fz_transform_point(&pt, &mtx);
	pos.set(pt.x, pt.y, 0);

	fz_point wid;
	wid.x = bbox.x1 - bbox.x0;
	wid.y = 0;
	fz_transform_vector(&wid, &mtx);
	boxWidth = sqrt(wid.x*wid.x + wid.y*wid.y);

	fz_point hi;
	hi.x = 0;
	hi.y = bbox.y1 - bbox.y0;
	fz_transform_vector(&hi, &mtx);
	boxHeight = sqrt(hi.x*hi.x + hi.y*hi.y);
}

void getCurTextProp(pdfapp_t* app, fz_display_node* pNode, int i, fz_matrix& mtx, AcGiTextStyle& tmpStyle, TextProp& prop)
{
	prop.str = (ACHAR)pNode->item.text->items[i].ucs;

	mtx.e = pNode->item.text->items[i].x;
	mtx.f = pNode->item.text->items[i].y;
	double boxHeight = 0;
	double boxWidth = 0;
	getBBoxFeature(app, pNode, i, mtx, prop.pos, boxHeight, boxWidth, prop.orignPos, prop.orignHeight);
	prop.height = boxHeight;

	tmpStyle.setTextSize(boxHeight);
	AcGePoint2d exts = tmpStyle.extents(prop.str, Adesk::kTrue, -1, Adesk::kTrue);
	double actHeight = max(exts.x, exts.y);
	double hFactor = 1;
	if (!REALEQ(actHeight, 0) && !REALEQ(boxHeight, 0))
	{
		hFactor = boxHeight / actHeight;
		if (hFactor > 1)
		{
			hFactor = 1;
			prop.height = actHeight;
		}
	}
	prop.hFactor = hFactor;

	//如果文字的实际大小本来就比bbox小就不用扩大了，这种情况一般是标点符号
	tmpStyle.setTextSize(prop.height * prop.hFactor);
	exts = tmpStyle.extents(prop.str, Adesk::kTrue, -1, Adesk::kTrue);
	prop.wFactor = 1;
	if ((boxWidth < exts.x) && (!REALEQ(exts.x, 0)))
	{
		prop.wFactor = boxWidth / exts.x;
		if (REALEQ(prop.wFactor, 0))
		{
			prop.wFactor = 1;
		}
	}

	//要靠下一个文字的位置来设置tracking
	AcGePoint3d nextPos;
	double minWidth = min(boxWidth, exts.x);
	if (((i+1) < pNode->item.text->len) && !REALEQ(minWidth, 0))
	{
		mtx.e = pNode->item.text->items[i+1].x;
		mtx.f = pNode->item.text->items[i+1].y;
		fz_point pt;
		double oh = 0;
		getBBoxFeature(app, pNode, i+1, mtx, nextPos, boxHeight, boxWidth, pt, oh);
		prop.tracking = prop.pos.distanceTo(nextPos) / minWidth;
	}
	else
	{
		prop.tracking = 1;
	}

//////////////////////////以下仅测试用/////////////////////////////////
/*
	mtx.e = pNode->item.text->items[i].x;
	mtx.f = pNode->item.text->items[i].y;
	fz_rect bbox;
	fz_bound_glyph(app->ctx, pNode->item.text->font, pNode->item.text->items[i].gid, &mtx, &bbox);
	AcDbPolyline* pPline = new AcDbPolyline;
	pPline->addVertexAt(0, AcGePoint2d(bbox.x0, bbox.y0));
	pPline->addVertexAt(0, AcGePoint2d(bbox.x1, bbox.y0));
	pPline->addVertexAt(0, AcGePoint2d(bbox.x1, bbox.y1));
	pPline->addVertexAt(0, AcGePoint2d(bbox.x0, bbox.y1));
	pPline->setClosed(kTrue);
	append2Db(app, pPline, FALSE);
	pPline->close();
*/
}

void appendStoke(AcString& msg, double num, const ACHAR* pCh, double defVal)
{
	CString token;
	if (!REALEQ(num, defVal))
	{
		if (msg.substr(1) != _T("{"))
		{
			msg = _T("{") + msg;
		}
		token.Format(CString(pCh) + _T("%.4lf"), num);
		token.TrimRight(_T("0"));
		token.TrimRight('.');
		token += ';';
//不知道为什么直接这么搞会导致之加上了一个\ 		msg += (LPCTSTR)token;
		AcString tmp = (LPCTSTR)token;
		msg += tmp;
	}
}

void AppendTxt2Db(pdfapp_t* app, AcString& str, AcDbObjectId& idTxtStyle, AcCmColor& color, AcGePoint3d pos, double rot)
{
	AcDbMText* pText = new AcDbMText;
	pText->setTextStyle(idTxtStyle);
	pText->setColor(color);
	pText->setContents(str);
	pText->setAttachment(AcDbMText::kBottomLeft);
	pText->setLocation(pos);
	pText->setRotation(rot);
	append2Db(app, pText, FALSE);
	pText->close();
}

AcString makeSegment(TextProp& prop)
{
	AcString ret = _T("");
	appendStoke(ret, prop.height * prop.hFactor, AcDbMText::heightChange(), 0);
	appendStoke(ret, prop.wFactor, AcDbMText::widthChange(), 1);
	appendStoke(ret, prop.tracking, AcDbMText::trackChange(), 1);
	CString cstr = prop.str.kACharPtr();
	cstr.Replace(_T("\\"), _T("\\\\"));
	AcString astr = (LPCTSTR)cstr;
	ret += astr;
	if (ret.substr(1) == _T("{"))
	{
		ret += _T("}");
	}
	return ret;
}

double calTextRotation(fz_matrix& mtx)
{
	AcGeMatrix2d mat;
	mat.setCoordSystem(AcGePoint2d(mtx.e, mtx.f), AcGeVector2d(mtx.a, mtx.c), AcGeVector2d(mtx.b, mtx.d));
	AcGeVector2d vec(1, 0);
	vec.transformBy(mat);
	return 2 * M_PI - vec.angle();
}

void creatText(pdfapp_t* app, fz_display_node* pNode)
{
	AcCmColor color = getColor(app, pNode);
	AcDbObjectId idTxtStyle;
	if (app->idStyle.isValid())
	{
		idTxtStyle = app->idStyle;
	}
	else
	{
		idTxtStyle = app->pDb->textstyle();
	}
	AcGiTextStyle tmpStyle;
	fromAcDbTextStyle(tmpStyle, idTxtStyle);
	fz_matrix mtx = pNode->item.text->trm;
	double expansion = fz_matrix_expansion(&mtx);
	double rot = calTextRotation(mtx);
	TextProp curProp;
	AcString curSegs;
	AcGePoint3d pos;
	if (pNode->item.text->len > 0)
	{
		getCurTextProp(app, pNode, 0, mtx, tmpStyle, curProp);
		pos = curProp.pos;
	}
	for (int i=1; i<pNode->item.text->len; i++)
	{
		TextProp prop;
		getCurTextProp(app, pNode, i, mtx, tmpStyle, prop);
		if (REALEQ(curProp.height, 0) || !REALEQ(curProp.orignPos.y, prop.orignPos.y, curProp.orignHeight / 5) || 
			     (curProp.tracking < 0.75) || (curProp.tracking > 4))
		{
			if (!REALEQ(curProp.height, 0))
			{
				curSegs += makeSegment(curProp);
			}
			if (curSegs.length() != 0)
			{
				AppendTxt2Db(app, curSegs, idTxtStyle, color, pos, rot);
				curSegs = _T("");
			}
			curProp = prop;
			pos = curProp.pos;
		}
		else if (!REALEQ(curProp.height, prop.height) || !REALEQ(curProp.wFactor, prop.wFactor) || 
			!REALEQ(curProp.tracking, prop.tracking, 0.1))
		{
			curSegs += makeSegment(curProp);
			curProp = prop;
		}
		else
		{
			curProp.str += prop.str;
		}
	}
	if ((curSegs.length() != 0) || (!REALEQ(curProp.height, 0)))
	{
		curSegs += makeSegment(curProp);
		AppendTxt2Db(app, curSegs, idTxtStyle, color, pos, rot);
	}
}

BOOL rectInsideClip(fz_rect& rectNode, fz_rect& rectClip)
{
	return ((rectNode.x0 >= rectClip.x0) && (rectNode.y0 >= rectClip.y0) && 
		    (rectNode.x1 <= rectClip.x1) && (rectNode.y1 <= rectClip.y1));
}

//简单处理一下clip, 只要不整体在第一层clip里面其余clip外面的一律不不转换
BOOL nodeInsideClip(fz_display_node* pNode, vector<int>& stkClipType, vector<fz_rect>& stkClipRect)
{
	BOOL ret = TRUE;
	BOOL bFirstClip = TRUE;

	for (int i=0; i<stkClipType.size(); i++)
	{
		if (stkClipType[i] == FZ_CMD_CLIP_IMAGE_MASK)
		{
			continue;
		}
		else if (stkClipType[i] == (pNode->cmd+2))
		{

			ret = rectInsideClip(pNode->rect, stkClipRect[i]);
			if (bFirstClip)
			{
				bFirstClip = FALSE;
			}
			else
			{
				ret = !ret;
			}
			if (!ret)
			{
				break;
			}
		}
	}

	return ret;
}

void pdfapp_interpret(pdfapp_t* app)
{
	fz_display_list* pList = app->page_list;
	fz_display_node* pNode = pList->first;
	int imgIdx = 1;
	std::map<Adesk::UInt16, std::vector<AcDbPolyline*>> mapLineSeg;
	vector<int> stkClipType;
	vector<fz_rect> stkClipRect;
	while (pNode)
	{
		switch (pNode->cmd)
		{
		case FZ_CMD_FILL_IMAGE:
			createImage(app, pNode, imgIdx);
			break;
		case FZ_CMD_FILL_PATH:
		case FZ_CMD_STROKE_PATH:
			if (nodeInsideClip(pNode, stkClipType, stkClipRect)) 
			{
				createPath(app, pNode, mapLineSeg);
			}
			break;
		case FZ_CMD_FILL_TEXT:
		case FZ_CMD_STROKE_TEXT:
			if (nodeInsideClip(pNode, stkClipType, stkClipRect))
			{
				creatText(app, pNode);
			}
			break;
		case FZ_CMD_CLIP_PATH:
		case FZ_CMD_CLIP_STROKE_PATH:
		case FZ_CMD_CLIP_TEXT:
		case FZ_CMD_CLIP_STROKE_TEXT:
		case FZ_CMD_CLIP_IMAGE_MASK:
			stkClipType.push_back(pNode->cmd);
			stkClipRect.push_back(pNode->rect);
			break;
		case FZ_CMD_POP_CLIP:
			stkClipType.pop_back();
			stkClipRect.pop_back();
			break;
// 		default:
// 			int i=0;
		}
		pNode = pNode->next;
	}
	if (app->options.m_bLineSegOpt)
	{
		std::map<Adesk::UInt16, std::vector<AcDbPolyline*>>::iterator itr = mapLineSeg.begin();
		while (itr != mapLineSeg.end())
		{
			//临时的小线段会在连接完以后被删除掉
			UserHelp::AppendPlines(itr->second, app->pDb);
			itr++;
		}
	}
}

void pdfapp_loadpage(pdfapp_t* app)
{
	fz_device* mdev = NULL;
	int errored = 0;
	fz_cookie cookie = { 0 };

	fz_var(mdev);

	fz_drop_display_list(app->ctx, app->page_list);
	fz_drop_display_list(app->ctx, app->annotations_list);
	fz_free_text_page(app->ctx, app->page_text);
	fz_free_text_sheet(app->ctx, app->page_sheet);
	fz_drop_link(app->ctx, app->page_links);
	fz_free_page(app->doc, app->page);

	app->page_list = NULL;
	app->annotations_list = NULL;
	app->page_text = NULL;
	app->page_sheet = NULL;
	app->page_links = NULL;
	app->page = NULL;
	app->page_bbox.x0 = 0;
	app->page_bbox.y0 = 0;
	app->page_bbox.x1 = 100;
	app->page_bbox.y1 = 100;

	app->incomplete = 0;

	fz_try(app->ctx)
	{
		app->page = fz_load_page(app->doc, app->pageno - 1);
		fz_bound_page(app->doc, app->page, &app->page_bbox);
		double midY = (app->page_bbox.y0 + app->page_bbox.y1) / 2;
		app->mtxMirror = AcGeMatrix3d::mirroring(AcGeLine3d(AcGePoint3d(app->page_bbox.x0, midY, 0), AcGePoint3d(app->page_bbox.x1, midY, 0)));
	}
	fz_catch(app->ctx)
	{
		if (fz_caught(app->ctx) == FZ_ERROR_TRYLATER)
		{
			app->incomplete = 1;
		}
		else
		{
			acedAlert(_T("Cannot load page"));
		}
		return;
	}

	fz_try(app->ctx)
	{
		fz_annot *annot;
		/* Create display lists */
		app->page_list = fz_new_display_list(app->ctx);
		mdev = fz_new_list_device(app->ctx, app->page_list);
		cookie.incomplete_ok = 1;
		fz_run_page_contents(app->doc, app->page, mdev, &fz_identity, &cookie);
		fz_free_device(mdev);
		mdev = NULL;
		pdfapp_interpret(app);
		app->annotations_list = fz_new_display_list(app->ctx);
		mdev = fz_new_list_device(app->ctx, app->annotations_list);
		for (annot = fz_first_annot(app->doc, app->page); annot; annot = fz_next_annot(app->doc, annot))
		{
			fz_run_annot(app->doc, app->page, annot, mdev, &fz_identity, &cookie);
		}
		if (cookie.incomplete)
		{
			app->incomplete = 1;
			acedAlert(_T("Incomplete page rendering"));
		}
		else if (cookie.errors)
		{
			acedAlert(_T("Errors found on page"));
			errored = 1;
		}
	}
	fz_always(app->ctx)
	{
		fz_free_device(mdev);
	}
	fz_catch(app->ctx)
	{
		if (fz_caught(app->ctx) == FZ_ERROR_TRYLATER)
		{
			app->incomplete = 1;
		}
		else
		{
			acedAlert(_T("Cannot load page"));
			errored = 1;
		}
	}

	fz_try(app->ctx)
	{
		app->page_links = fz_load_links(app->doc, app->page);
	}
	fz_catch(app->ctx)
	{
		if (fz_caught(app->ctx) == FZ_ERROR_TRYLATER)
		{
			app->incomplete = 1;
		}
		else if (!errored)
		{
			acedAlert(_T("Cannot load page"));
		}
	}

	app->errored = errored;
}

void pdfapp_convertCurPage(pdfapp_t* app)
{
	AcString outputName;
	ACHAR out[_MAX_PATH];
	removeExt(app->strFileName.kACharPtr(), out);
	bool singlePage = (app->pageno > 0);
	app->pageno = abs(app->pageno);
	if (app->bOutputDwg)
	{
		outputName.format(_T("%s_%d.DWG"), out, app->pageno);
	}
	else
	{
		outputName.format(_T("%s_%d.DXF"), out, app->pageno);
	}
	AcDbDatabase* pDb = new AcDbDatabase();
	app->pDb = pDb;
	AcDbTextStyleTable* pTbl = NULL;
	pDb->getSymbolTable(pTbl, AcDb::kForWrite);
	if (pTbl)
	{
		AcDbTextStyleTableRecord* pRec = new AcDbTextStyleTableRecord();
		static int s_newStyleNameIdx = 0;
		pRec->setName(_T("DefaultStyle"));
		pRec->setFileName(_T("arial.ttf"));
		pTbl->add(app->idStyle, pRec);
		pTbl->close();
		if (app->idStyle.isValid())
		{
			pRec->close();
		}
		else
		{
			ASSERT(FALSE);
			delete pRec;
		}
		pTbl->close();
	}

	pdfapp_loadpage(app);

	if (app->bOutputDwg)
	{
		pDb->saveAs(outputName, false);
	}
	else
	{
		pDb->dxfOut(outputName);
	}
	delete pDb;
	app->pDb = NULL;

	if (singlePage)
	{
		CString strPrompt;
		CAcModuleResourceOverride rs;
		strPrompt.LoadString(IDS_INSERT2CUR);
		acedInitGet(0, _T("Yes No"));
		ACHAR kword[32];
		int rc = acedGetKword((LPCTSTR)strPrompt, kword);
		if ((RTCAN == rc) || (!_tcscmp(kword, _T("No"))))
		{
			return;
		}

		resbuf rb;
		rb.restype = RTSHORT;
		rb.resval.rint = 0;
		acedSetVar(_T("CMDECHO"), &rb);

		strPrompt.LoadString(IDS_PICKINSPT);
		acutPrintf((LPCTSTR)strPrompt);
		acedCommand(RTSTR, _T("_Insert"), RTSTR, outputName.kACharPtr(), RTSTR, PAUSE, RTSTR, _T(""), RTSTR, _T(""), RTSTR, _T(""), RTNONE);

		rb.resval.rint = 1;
		acedSetVar(_T("CMDECHO"), &rb);
	}
}

void pdfapp_open(pdfapp_t* app, char* filename, int reload)
{
	fz_context* ctx = app->ctx;
	AcString password = _T("");
	int rc = 0;
	fz_var(rc);

	fz_try(ctx)
	{
		app->doc = fz_open_document(ctx, filename);

		char utf8Name[PATH_MAX];
		if (fz_needs_password(app->doc))
		{
			pdfapp_AcString2Utf8Char(password, utf8Name);
			int okay = fz_authenticate_password(app->doc, utf8Name);
			if (!okay)
			{
				AcString msg;
				ACHAR tmp[PATH_MAX];
				pdfapp_Utf8Char2AcString(filename, tmp);
				msg.format(_T("The file <%s> is encrypted. \nPlease input the password."), tmp);
				acedAlert(msg);
			}
			while (!okay)
			{
				password = pdfapp_password();
				if (password.length() == 0)
				{
					acedAlert(_T("Needs a password"));
					fz_rethrow(app->ctx);
				}
				pdfapp_AcString2Utf8Char(password, utf8Name);
				okay = fz_authenticate_password(app->doc, utf8Name);
				if (!okay)
				{
					acedAlert(_T("Invalid password."));
				}
			}
		}

		app->docpath = fz_strdup(ctx, filename);
		app->doctitle = filename;
		if (strrchr(app->doctitle, '\\'))
		{
			app->doctitle = strrchr(app->doctitle, '\\') + 1;
		}
		if (strrchr(app->doctitle, '/'))
		{
			app->doctitle = strrchr(app->doctitle, '/') + 1;
		}
		app->doctitle = fz_strdup(ctx, app->doctitle);

		app->pagecount = fz_count_pages(app->doc);
		app->outline = fz_load_outline(app->doc);
	}
	fz_catch(ctx)
	{
		acedAlert(_T("cannot open document"));
		rc = 1;
	}

	if (rc)
	{
		return;
	}


	if (app->pagecount == 1)
		app->pageno = 1;
	if (app->pageno > app->pagecount)
		app->pageno = app->pagecount;
	if (app->resolution < MINRES)
		app->resolution = MINRES;
	if (app->resolution > MAXRES)
		app->resolution = MAXRES;

	if (!reload)
	{
		app->shrinkwrap = 1;
		app->rotate = 0;
		app->panx = 0;
		app->pany = 0;
	}

	if (app->pageno > 0)
	{
		pdfapp_convertCurPage(app);
	}
	else
	{
		AcString msg;
		msg.format(_T("Converting %s"), app->strFileName.kACharPtr());
		acedRestoreStatusBar();
		acedSetStatusBarProgressMeter(msg, 0, app->pagecount);
		for (int i=1; i<=app->pagecount; i++)
		{
			app->pageno = -1 * i;//把页数变成负的是为了判断是否正在转换全部页，如是则不要在转换后提示是否插入
			pdfapp_convertCurPage(app);
			acedSetStatusBarProgressMeterPos(-1);
		}
	}
}

void pdfapp_convert(pdfapp_t* gapp)
{
	char utf8Name[PATH_MAX];
	pdfapp_AcString2Utf8Char(gapp->strFileName, utf8Name);
	pdfapp_open(gapp, utf8Name, 0);
}