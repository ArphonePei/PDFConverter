#include "fitz_CPP.h"


class AcDbDatabase;

class CPdfappOptions
{
public:
	void init()
	{
		m_bArcOpt = TRUE;
		m_bLineSegOpt = FALSE;//这个对效率影响较大，默认关闭
	}

	BOOL m_bArcOpt;
	BOOL m_bLineSegOpt;
	std::map<Adesk::UInt16, std::vector<AcDbPolyline*>> m_mapLineSeg;
};

struct pdfapp_s
{
	/* current document params */
	fz_document *doc;
	char *docpath;
	char *doctitle;
	fz_outline *outline;
	int outline_deferred;

	int pagecount;

	/* current view params */
	int resolution;
	int rotate;
	fz_pixmap *image;
	int grayscale;
	fz_colorspace *colorspace;
	int invert;

	/* presentation mode */
	int presentation_mode;
	int transitions_enabled;
	fz_pixmap *old_image;
	fz_pixmap *new_image;
	clock_t start_time;
	int in_transit;
	float duration;
	fz_transition transition;

	/* current page params */
	int pageno;
	fz_page *page;
	fz_rect page_bbox;
	fz_display_list *page_list;
	fz_display_list *annotations_list;
	fz_text_page *page_text;
	fz_text_sheet *page_sheet;
	fz_link *page_links;
	int errored;
	int incomplete;

	/* snapback history */
	int hist[256];
	int histlen;
	int marks[10];

	/* window system sizes */
	int winw, winh;
	int scrw, scrh;
	int shrinkwrap;
	int fullscreen;

	/* event handling state */
	char number[256];
	int numberlen;

	int ispanning;
	int panx, pany;

	int iscopying;
	int selx, sely;
	/* TODO - While sely keeps track of the relative change in
	 * cursor position between two ticks/events, beyondy shall keep
	 * track of the relative change in cursor position from the
	 * point where the user hits a scrolling limit. This is ugly.
	 * Used in pdfapp.c:pdfapp_onmouse.
	 */
	int beyondy;
	fz_rect selr;

	int nowaitcursor;

	/* search state */
	int isediting;
	int searchdir;
	char search[512];
	int searchpage;
	fz_rect hit_bbox[512];
	int hit_count;

	/* client context storage */
	void *userdata;

	fz_context *ctx;
#ifdef HAVE_CURL
	fz_stream *stream;
#endif

	AcDbDatabase* pDb;
	AcString strFileName;
	CPdfappOptions options;
	AcDbObjectId idStyle;
	BOOL bOutputDwg;
	AcGeMatrix3d mtxMirror;
};

typedef struct pdfapp_s pdfapp_t;

