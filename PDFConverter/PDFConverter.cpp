
#include "stdafx.h"
#include "Windows.h"

#include "AcExtensionModule.h"
#include "pdfapp.h"
#include "resource.h"

void render(char *filename, int pagenumber, int zoom, int rotation);
void pdfapp_AcString2Utf8Char(AcString& str, char* utf8Name, int len=PATH_MAX);
void pdfapp_init(fz_context* ctx, pdfapp_t* app);
void pdfapp_close(pdfapp_t* app);
void pdfapp_open(pdfapp_t* app, char* filename, int reload);

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

extern "C" HWND adsw_acadMainWnd();

AC_IMPLEMENT_EXTENSION_MODULE(theArxDLL);

int CALLBACK EnumFontFamExProcGetRasterFontSize(  
	    ENUMLOGFONTEX *lpelfe,    // logical-font data  
	    NEWTEXTMETRICEX *lpntme,  // physical-font data  
	    DWORD FontType,           // type of font  
	    LPARAM lParam             // application-defined data  
	    )  
{
	return 1;  
}

void test()
{
	LOGFONT logFont;  
	memset(&logFont, 0x00, sizeof(logFont));  
	_tcscpy(logFont.lfFaceName, _T("Mic32New"));  
	logFont.lfCharSet = DEFAULT_CHARSET;
	HDC hdc = ::GetWindowDC(adsw_acadMainWnd());
	EnumFontFamiliesEx(hdc, &logFont,(FONTENUMPROC)EnumFontFamExProcGetRasterFontSize, 0, 0);
	::ReleaseDC(adsw_acadMainWnd(), hdc);
}

resbuf* getFileNameInput()
{
	BOOL ret = FALSE;
	resbuf* rbFileName = NULL;
	if(acedGetFileNavDialog(NULL, NULL, _T("pdf"), NULL, 4096, &rbFileName) == RTNORM)
	{
		return rbFileName;
	}
	else
	{
		return NULL;
	}
}

void convertFile(AcString pdfName, int page, BOOL bDwg)
{
	fz_context *ctx;
	ctx = fz_new_context(NULL, NULL, FZ_STORE_DEFAULT);
	if (!ctx)
	{
		return;
	}
	pdfapp_t gapp;
	pdfapp_init(ctx, &gapp);

	gapp.strFileName = pdfName;
	gapp.pageno = page;
	gapp.bOutputDwg = bDwg;


	//½øÐÐ×ª»»
	char utf8Name[PATH_MAX];
	pdfapp_AcString2Utf8Char(gapp.strFileName, utf8Name);
	pdfapp_open(&gapp, utf8Name, 0);

	pdfapp_close(&gapp);
	fz_free_context(ctx);
}

void PDFConverter(BOOL bDwg)
{
// 	test();
	resbuf* rb = getFileNameInput();
	resbuf* pRb = rb;
	AcStringArray arrFileName;
	while (pRb)
	{
		arrFileName.append(pRb->resval.rstring);
		pRb = pRb->rbnext;
	}
	acutRelRb(rb);
	rb = NULL;

	int page = 0;
	if (arrFileName.length() == 1)
	{
		acedInitGet(4, NULL);

		// use resource for multi langeage.
		// modify by yhl, 2016/6/29.
		CString strPrompt;
		CAcModuleResourceOverride rs;
		strPrompt.LoadString(IDS_ASKFORPAGENUMBER);
		int rc = acedGetInt(strPrompt, &page);
// 		int rc = acedGetInt(_T("\nPlease input the page number, 0 for all <1>: "), &page);

		if (RTNONE == rc)
		{
			page = 1;
		}
		else if (RTNORM != rc)
		{
			return;
		}
	}

	for (int i=0; i<arrFileName.length(); i++)
	{
		convertFile(arrFileName[i], page, bDwg);
	}
}

void PDF2DWG()
{
	PDFConverter(TRUE);
}

void PDF2DXF()
{
	PDFConverter(FALSE);
}

struct Buffer
{
	unsigned char *data;
	int length;
};

bool createSVGDataStr( AcString& filenamePDF, Buffer & outputBuffer )
{
	fz_context* ctx = nullptr;
	ctx = fz_new_context(NULL, NULL, FZ_STORE_DEFAULT);
	if (nullptr == ctx)
	{
		return false;
	}

	fz_document* doc = nullptr;
	fz_try(ctx)
	{
		char utf8Name[PATH_MAX];
		pdfapp_AcString2Utf8Char(filenamePDF, utf8Name);
		doc = fz_open_document(ctx, utf8Name);
	}
	fz_catch(ctx)
	{ 
		fz_free_context(ctx);
		return false;
	}

	int pageCount = fz_count_pages(doc);
	if (pageCount < 1)
		return false;

	fz_page* page = fz_load_page(doc, 0);

	fz_rect brect;
	fz_bound_page(doc, page, &brect);

	fz_buffer* buffer = fz_new_buffer(ctx, 1024);
	fz_output* output = fz_new_output_with_buffer(ctx, buffer);
	fz_device* device = fz_new_svg_device(ctx, output, brect.x1 - brect.x0, brect.y1 - brect.y0);

	fz_run_page(doc, page, device, &fz_identity, NULL);

	outputBuffer.data   = buffer->data;
	outputBuffer.length = buffer->len;

	fz_free_device(device);
	fz_close_output(output);
	fz_free_page(doc, page);
	fz_free_context(ctx);

	return true;
}

void PDF2SVG()
{
	resbuf* rb = getFileNameInput();
	resbuf* pRb = rb;
	AcStringArray arrFileName;
	while (pRb)
	{
		arrFileName.append(pRb->resval.rstring);
		pRb = pRb->rbnext;
	}
	acutRelRb(rb);
	rb = NULL;

	Buffer svgBuffer;
	if (!createSVGDataStr(arrFileName[0], svgBuffer))
	{
		acutPrintf(_T("\nFail to convert to SVG."));
		return;
	}

	AcString outputName;
	outputName.format(_T("%s.svg"), arrFileName[0].kACharPtr());
	CFile file;
	file.Open(outputName.kACharPtr(), CFile::modeCreate | CFile::modeWrite);
	CString outData = svgBuffer.data;
	file.Write((LPCTSTR)outData, outData.GetLength()*sizeof(TCHAR));
	file.Close();
	acutPrintf(_T("\nDone."));
}

void initApp()
{ 
	// register a command with the AutoCAD command mechanism
	CAcModuleResourceOverride resOverride;

	acedRegCmds->addCommand(_T("ZOSP_PDF_TOOLs"), 
		_T("PDF2DWG"), 
		_T("PDF2DWG"), 
		ACRX_CMD_MODAL, 
		PDF2DWG,
		NULL,
		-1,
		theArxDLL.ModuleResourceInstance());

	acedRegCmds->addCommand(_T("ZOSP_PDF_TOOLs"), 
		_T("PDF2DXF"), 
		_T("PDF2DXF"), 
		ACRX_CMD_MODAL, 
		PDF2DXF,
		NULL,
		-1,
		theArxDLL.ModuleResourceInstance());

	acedRegCmds->addCommand(_T("ZOSP_PDF_TOOLs"), 
		_T("PDF2SVG"), 
		_T("PDF2SVG"), 
		ACRX_CMD_MODAL, 
		PDF2SVG,
		NULL,
		-1,
		theArxDLL.ModuleResourceInstance());

	// use resource for multi langeage.
	// modify by yhl, 2016/6/29.
	CString strGpl;
	strGpl.LoadString(IDS_GPLDECLAREMENT);
	acutPrintf(strGpl);
// 	acutPrintf(_T("\nThis application is released under the terms of GNU-GPL v3."));
// 	acutPrintf(_T("\nYou can access its source code here: <https://github.com/ArphonePei/PDFConverter>"));
}

void unloadApp()
{ 
	acedRegCmds->removeGroup(_T("ZOSP_PDF_TOOLs"));  
}

//////////////////////////////////////////////////////////////
//
// Entry points
//
//////////////////////////////////////////////////////////////
extern "C" int APIENTRY
	DllMain(HINSTANCE hInstance, DWORD dwReason, LPVOID lpReserved)
{
	// Remove this if you use lpReserved
	UNREFERENCED_PARAMETER(lpReserved);

	if (dwReason == DLL_PROCESS_ATTACH)
	{
		theArxDLL.AttachInstance(hInstance);
	}
	else if (dwReason == DLL_PROCESS_DETACH)
	{
		theArxDLL.DetachInstance();  
	}
	return 1;   // ok
}

extern "C" AcRx::AppRetCode zcrxEntryPoint( AcRx::AppMsgCode msg, void* appId)
{
	switch( msg ) 
	{
	case AcRx::kInitAppMsg: 
		acrxDynamicLinker->unlockApplication(appId);
		acrxDynamicLinker->registerAppMDIAware(appId);
		initApp(); 
		break;
	case AcRx::kUnloadAppMsg: 
		unloadApp(); 
		break;
	case AcRx::kInitDialogMsg:

		break;
	default:
		break;
	}
	return AcRx::kRetOK;
}