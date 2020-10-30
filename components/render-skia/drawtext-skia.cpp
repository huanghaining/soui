#include "drawtext-skia.h"

#define DT_ELLIPSIS (DT_PATH_ELLIPSIS|DT_END_ELLIPSIS|DT_WORD_ELLIPSIS)
#define CH_ELLIPSIS L"..."
#define MAX(a,b)    (((a) > (b)) ? (a) : (b))

static size_t breakTextEx(const SkPaint *pPaint, const wchar_t* textD, size_t length, SkScalar maxWidth,
                          SkScalar* measuredWidth) 
{
    size_t nLineLen=pPaint->breakText(textD,length*sizeof(wchar_t),maxWidth,measuredWidth,SkPaint::kForward_TextBufferDirection);
    if(nLineLen==0) return 0;
    nLineLen/=sizeof(wchar_t);

    const wchar_t * p=textD;
    for(size_t i=0;i<nLineLen;i++, p++)
    {
        if(*p == L'\r')
        {
            if(i<nLineLen-1 && p[1]==L'\n') return i+2;
            else return i;
        }else if(*p == L'\n')
        {
            return i+1;
        }
    }
    return nLineLen;
}
#include <xstring>
#include <set>
using namespace std;
bool IsEmoji(const std::wstring& wstrText);
SkRect DrawText_Skia(SkCanvas* canvas,const wchar_t *text,int len,SkRect box,const SkPaint& paint,UINT uFormat)
{
	if(len<0)	len = wcslen(text);
    SkTextLayoutEx layout;
	if (IsEmoji(text))
	{
		
		len = len;
	}
    layout.init(text,len,box,paint,uFormat);
    
    return layout.draw(canvas);
}

bool IsEmoji(const std::wstring& wstrText)
{
	// 去掉一些暂时无法显示的emoji
	bool emoji = false;
	std::wstring wstrTextTemp(L"");
	const std::wstring strFlag(L" ");   //emoji表情用空格代替
	const std::set<unsigned int> emojiSet = { 0x231a, 0x231b, 0x23e9, 0x23ea, 0x23eb, 0x23ec, 0x23f0, 0x23f3, 0x25fd, 0x25fe,
		0x2614, 0x2615, 0x2648, 0x2649, 0x264a, 0x264b, 0x264c, 0x264d, 0x264e, 0x264f, 0x2650, 0x2651, 0x2652, 0x2653, 0x267f, 0x2693, 0x26a1, 0x26aa,
		0x26ab, 0x26bd, 0x26be, 0x26c4, 0x26c5, 0x26ce, 0x26d4, 0x26ea, 0x26f2, 0x26f3, 0x26f5, 0x26fa, 0x26fd, 0x2705, 0x270a, 0x270b, 0x2728, 0x274c,
		0x274e, 0x2753, 0x2754, 0x2755, 0x2757, 0x2795, 0x2796, 0x2797, 0x27b0, 0x27bf, 0x2b1b, 0x2b1c, 0x2b50, 0x2b55 };

	for (std::wstring::size_type nIndex = 0; nIndex < wstrText.size(); ++nIndex)
	{
		wchar_t wChar = wstrText[nIndex];
		if (wChar == 0xfe0f)    //三字节emoji表情的第3个字节 65039
		{
			//OMLOG_BEGIN(LEVEL_MONITOR, GID_NNC) << "NNC_RichTextLogID=1902 FTRichTextSnippetBase::NormalizeString, codeNum1 = " << (unsigned int)wChar << OMLOG_END;
			continue;
			emoji = true;
		}

		if (wChar < 0xD800 || wChar > 0xDFFF)
		{
			if (emojiSet.find(wChar) != emojiSet.end())//单字节emoji表情
			{
				//	OMLOG_BEGIN(LEVEL_MONITOR, GID_NNC) << "NNC_RichTextLogID=1900 FTRichTextSnippetBase::NormalizeString, codeNum1 = " << (unsigned int)wChar << OMLOG_END;
				wstrTextTemp.append(strFlag);
				emoji = true;
			}
			else
			{
				if (nIndex + 1 < wstrText.size() && wstrText[nIndex + 1] == 0xfe0f)//双字节emoji表情
				{
					//	OMLOG_BEGIN(LEVEL_MONITOR, GID_NNC) << "NNC_RichTextLogID=1901 FTRichTextSnippetBase::NormalizeString, codeNum1 = " << (unsigned int)wChar << ", codeNum2 =" << (unsigned int)wstrText[nIndex + 1] << OMLOG_END;
					++nIndex;
					wstrTextTemp.append(strFlag);
					emoji = true;
				}
				else
				{
					wstrTextTemp += wChar;
				}
			}
		}
		else
		{
			if (wChar >= 0xD800 && wChar <= 0xDBFF)
			{
				// 				if (nIndex + 1 < wstrText.size())
				// 				{
				// 					OMLOG_BEGIN(LEVEL_MONITOR, GID_NNC) << "NNC_RichTextLogID=1902 FTRichTextSnippetBase::NormalizeString, codeNum1 = " << (unsigned int)wChar << ", codeNum2 =" << (unsigned int)wstrText[nIndex + 1] << OMLOG_END;
				// 				}
				emoji = true;
				++nIndex;
				wstrTextTemp.append(strFlag);
			}
		}
	}
	return emoji;
}


//////////////////////////////////////////////////////////////////////////
void SkTextLayoutEx::init( const wchar_t text[], size_t length,SkRect rc, const SkPaint &paint,UINT uFormat )
{
    if(uFormat & DT_NOPREFIX)
    {
        m_text.setCount(length);
        memcpy(m_text.begin(),text,length*sizeof(wchar_t));
    }else
    {
        m_prefix.reset();
        SkTDArray<wchar_t> tmp;
        tmp.setCount(length);
        memcpy(tmp.begin(),text,length*sizeof(wchar_t));
        for(int i=0;i<tmp.count();i++)
        {
            if(tmp[i]==L'&' && i+1 < tmp.count())
            {
				if(tmp[i+1]==L'&')
				{
					tmp.remove(i,1);
				}else
				{
					tmp.remove(i,1);
					m_prefix.push(i);
				}
            }
        }
        m_text=tmp;
    }

    m_paint=&paint;
    m_rcBound=rc;
    m_uFormat=uFormat;
    buildLines();
}

void SkTextLayoutEx::buildLines()
{
    m_lines.reset();

    if(m_uFormat & DT_SINGLELINE)
    {
        m_lines.push(0);
    }else
    {
        const wchar_t *text = m_text.begin();
        const wchar_t* stop = m_text.begin() + m_text.count();
        SkScalar maxWid=m_rcBound.width();
        if(m_uFormat & DT_CALCRECT && maxWid < 1.0f)
            maxWid=10000.0f;
        int lineHead=0;
        while(lineHead<m_text.count())
        {
            m_lines.push(lineHead);
            size_t line_len = breakTextEx(m_paint,text, stop - text, maxWid,0);
			if (0 == line_len) break;
			text += line_len;
            lineHead += line_len;
        };
    }
}

SkScalar SkTextLayoutEx::drawLine( SkCanvas *canvas, SkScalar x, SkScalar y, int iBegin,int iEnd )
{
    const wchar_t *text=m_text.begin()+iBegin;

    if(!(m_uFormat & DT_CALCRECT))
    {
        canvas->drawText(text,(iEnd-iBegin)*sizeof(wchar_t),x,y,*m_paint);
        int i=0;
        while(i<m_prefix.count())
        {
            if(m_prefix[i]>=iBegin)
                break;
            i++;
        }
        
        SkScalar xBase = x;
        if(m_paint->getTextAlign() != SkPaint::kLeft_Align)
        {
            SkScalar nTextWidth = m_paint->measureText(text,(iEnd-iBegin)*sizeof(wchar_t));
            switch(m_paint->getTextAlign())
            {
            case SkPaint::kCenter_Align:
                xBase = x - nTextWidth/2.0f;
                break;
            case SkPaint::kRight_Align:
                xBase = x- nTextWidth;
                break;
            }
        }
        
        while(i<m_prefix.count() && m_prefix[i]<iEnd)
        {
            SkScalar x1 = m_paint->measureText(text,(m_prefix[i]-iBegin)*sizeof(wchar_t));
            SkScalar x2 = m_paint->measureText(text,(m_prefix[i]-iBegin+1)*sizeof(wchar_t));
            canvas->drawLine(xBase+x1,y+1,xBase+x2,y+1,*m_paint); //绘制下划线
            i++;
        }
    }
    return m_paint->measureText(text,(iEnd-iBegin)*sizeof(wchar_t));
}

SkScalar SkTextLayoutEx::drawLineEndWithEllipsis( SkCanvas *canvas, SkScalar x, SkScalar y, int iBegin,int iEnd,SkScalar maxWidth )
{
    SkScalar widReq=m_paint->measureText(m_text.begin()+iBegin,(iEnd-iBegin)*sizeof(wchar_t));
    if(widReq<=m_rcBound.width())
    {
        return drawLine(canvas,x,y,iBegin,iEnd);
    }else
    {
        SkScalar fWidEllipsis = m_paint->measureText(CH_ELLIPSIS,sizeof(CH_ELLIPSIS)-sizeof(wchar_t));
        maxWidth-=fWidEllipsis;
        
        int i=0;
        const wchar_t *text=m_text.begin()+iBegin;
        SkScalar fWid=0.0f;
        while(i<(iEnd-iBegin))
        {
            SkScalar fWord = m_paint->measureText(text+i,sizeof(wchar_t));
            if(fWid + fWord > maxWidth) break;
            fWid += fWord;
            i++;
        }
        if(!(m_uFormat & DT_CALCRECT))
        {
            wchar_t *pbuf=new wchar_t[i+3];
            memcpy(pbuf,text,i*sizeof(wchar_t));
            memcpy(pbuf+i,CH_ELLIPSIS,3*sizeof(wchar_t));
            canvas->drawText(pbuf,(i+3)*sizeof(wchar_t),x,y,*m_paint);
            delete []pbuf;
        }
        return fWid+fWidEllipsis;
    }
}

SkRect SkTextLayoutEx::draw( SkCanvas* canvas )
{
    SkPaint::FontMetrics metrics;

    m_paint->getFontMetrics(&metrics);
    float lineSpan = metrics.fBottom-metrics.fTop;

    SkRect rcDraw = m_rcBound;

    float  x;
    switch (m_paint->getTextAlign()) 
    {
    case SkPaint::kCenter_Align:
        x = SkScalarHalf(m_rcBound.width());
        break;
    case SkPaint::kRight_Align:
        x = m_rcBound.width();
        break;
    default://SkPaint::kLeft_Align:
        x = 0;
        break;
    }
    x += m_rcBound.fLeft;

    canvas->save();

    canvas->clipRect(m_rcBound);

    float height = m_rcBound.height();
    float y=m_rcBound.fTop - metrics.fTop;
    if(m_uFormat & DT_SINGLELINE)
    {//单行显示
        rcDraw.fBottom = rcDraw.fTop + lineSpan;
        if(m_uFormat & DT_VCENTER) 
        {
            y += (height - lineSpan)/2.0f;
        }
		else if (m_uFormat & DT_BOTTOM)
		{
			y += (height - lineSpan);
		}
        if(m_uFormat & DT_ELLIPSIS)
        {//只支持在行尾增加省略号
            rcDraw.fRight = rcDraw.fLeft + drawLineEndWithEllipsis(canvas,x,y,0,m_text.count(),m_rcBound.width());
        }else
        {
            rcDraw.fRight = rcDraw.fLeft + drawLine(canvas,x,y,0,m_text.count());
        }
    }else
    {//多行显示
        SkScalar maxLineWid=0;
        int iLine = 0;
        while(iLine<m_lines.count())
        {
            if(y + lineSpan + metrics.fTop >= m_rcBound.fBottom) 
                break;  //the last visible line
            int iBegin=m_lines[iLine];
            int iEnd = iLine<(m_lines.count()-1)?m_lines[iLine+1]:m_text.count();
            SkScalar lineWid = drawLine(canvas,x,y,iBegin,iEnd);
            maxLineWid = MAX(maxLineWid,lineWid);
            y += lineSpan;
            iLine ++;
        }
        if(iLine<m_lines.count())
        {//draw the last visible line
            int iBegin=m_lines[iLine];
            int iEnd = iLine<(m_lines.count()-1)?m_lines[iLine+1]:m_text.count();
            SkScalar lineWid;
            if(m_uFormat & DT_ELLIPSIS)
            {//只支持在行尾增加省略号
                lineWid=drawLineEndWithEllipsis(canvas,x,y,iBegin,iEnd,m_rcBound.width());
            }else
            {
                lineWid=drawLine(canvas,x,y,iBegin,iEnd);
            }
            maxLineWid = MAX(maxLineWid,lineWid);
            y += lineSpan;
        }
        rcDraw.fRight = rcDraw.fLeft + maxLineWid;
        rcDraw.fBottom = y + metrics.fTop;
    }
    canvas->restore();
    return rcDraw;
}
