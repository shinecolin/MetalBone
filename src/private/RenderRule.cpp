#include "MResource.h"
#include "MWidget.h"
#include "MApplication.h"
#include "MD2DPaintContext.h"
#include "private/CSSParser.h"
#include "private/RenderRuleData.h"

#include <dwrite.h>
#include <algorithm>

#ifndef MB_NO_GDIPLUS
#  include <gdiplus.h>
#endif

namespace MetalBone
{
    // ========== XXRenderObject ==========
    namespace CSS
    {
        template<class T>
        inline bool testValue(const T* t, ValueType value)
        {
            M_ASSERT(value < Value_BitFlagsEnd);
            return (t->values & value) != 0;
        }
        template<class T>
        inline bool testNoValue(const T* t, ValueType value)
        {
            M_ASSERT(value < Value_BitFlagsEnd);;
            return (t->values & value) == 0;
        }

        ComplexBorderRenderObject::ComplexBorderRenderObject():uniform(false),isTransparent(true)
        {
            type = ComplexBorder;
            styles.left = styles.top = styles.right = styles.bottom = Value_Solid;
            memset(&widths  , 0, sizeof(MRectU));
            memset(&radiuses, 0, 4 * sizeof(unsigned int));
        }
        void ComplexBorderRenderObject::getBorderWidth(MRect& rect) const
        {
            rect.left   = widths.left;
            rect.right  = widths.right;
            rect.bottom = widths.bottom;
            rect.top    = widths.top;
        }
        bool ComplexBorderRenderObject::isVisible() const
        {
            if(isTransparent) return false;
            if(widths.left == 0 && widths.right == 0 && widths.top == 0 && widths.bottom == 0)
                return false;
            return true;
        }
        ID2D1Geometry* ComplexBorderRenderObject::createGeometry(const D2D1_RECT_F& rect)
        {
            ID2D1GeometrySink* sink;
            ID2D1PathGeometry* pathGeo;

            FLOAT ft = rect.top    + widths.top    / 2;
            FLOAT fr = rect.right  + widths.right  / 2;
            FLOAT fl = rect.left   + widths.left   / 2;
            FLOAT fb = rect.bottom + widths.bottom / 2;

            mApp->getD2D1Factory()->CreatePathGeometry(&pathGeo);
            pathGeo->Open(&sink);
            sink->BeginFigure(D2D1::Point2F(rect.left + radiuses[0],ft),D2D1_FIGURE_BEGIN_FILLED);

            sink->AddLine(D2D1::Point2F(rect.right - radiuses[1], ft));
            if(radiuses[1] != 0)
                sink->AddArc(D2D1::ArcSegment(
                D2D1::Point2F(fr, ft + radiuses[1]),
                D2D1::SizeF((FLOAT)radiuses[1],(FLOAT)radiuses[1]),
                0.f,D2D1_SWEEP_DIRECTION_CLOCKWISE,D2D1_ARC_SIZE_SMALL));
            sink->AddLine(D2D1::Point2F(fr, fb - radiuses[2]));
            if(radiuses[2] != 0)
                sink->AddArc(D2D1::ArcSegment(
                D2D1::Point2F(fr - radiuses[2], fb),
                D2D1::SizeF((FLOAT)radiuses[2],(FLOAT)radiuses[2]),
                0.f,D2D1_SWEEP_DIRECTION_CLOCKWISE,D2D1_ARC_SIZE_SMALL));
            sink->AddLine(D2D1::Point2F(fl + radiuses[3], fb));
            if(radiuses[3] != 0)
                sink->AddArc(D2D1::ArcSegment(
                D2D1::Point2F(fl, fb - radiuses[3]),
                D2D1::SizeF((FLOAT)radiuses[3],(FLOAT)radiuses[3]),
                0.f,D2D1_SWEEP_DIRECTION_CLOCKWISE,D2D1_ARC_SIZE_SMALL));
            sink->AddLine(D2D1::Point2F(fl, ft + radiuses[0]));
            if(radiuses[0] != 0)
                sink->AddArc(D2D1::ArcSegment(
                D2D1::Point2F(fl + radiuses[0], ft),
                D2D1::SizeF((FLOAT)radiuses[0],(FLOAT)radiuses[0]),
                0.f,D2D1_SWEEP_DIRECTION_CLOCKWISE,D2D1_ARC_SIZE_SMALL));

            sink->EndFigure(D2D1_FIGURE_END_CLOSED);
            sink->Close();
            SafeRelease(sink);
            return pathGeo; 
        }
        void ComplexBorderRenderObject::draw(ID2D1RenderTarget* rt,ID2D1Geometry* geo,const D2D1_RECT_F& rect)
        {
            if(uniform) {
                rt->DrawGeometry(geo,brushes[0],(FLOAT)widths.left);
                return;
            }

            FLOAT fl = rect.left   + widths.left   / 2;
            FLOAT ft = rect.top    + widths.top    / 2;
            FLOAT fr = rect.right  + widths.right  / 2;
            FLOAT fb = rect.bottom + widths.bottom / 2;

            if(widths.top > 0)
            {
                FLOAT offset = (FLOAT)widths.top / 2;
                if(radiuses[0] != 0 || radiuses[1] != 0)
                {
                    ID2D1PathGeometry* pathGeo;
                    ID2D1GeometrySink* sink;
                    mApp->getD2D1Factory()->CreatePathGeometry(&pathGeo);
                    pathGeo->Open(&sink);

                    if(radiuses[0] != 0) {
                        sink->BeginFigure(D2D1::Point2F(fl, ft + radiuses[0]),D2D1_FIGURE_BEGIN_FILLED);
                        sink->AddArc(D2D1::ArcSegment(
                            D2D1::Point2F(rect.left + radiuses[0] + offset, ft),
                            D2D1::SizeF((FLOAT)radiuses[0],(FLOAT)radiuses[0]),
                            0.f,D2D1_SWEEP_DIRECTION_CLOCKWISE,D2D1_ARC_SIZE_SMALL));
                    } else {
                        sink->BeginFigure(D2D1::Point2F(rect.left,ft),D2D1_FIGURE_BEGIN_FILLED);
                    }

                    if(radiuses[1] != 0) {
                        sink->AddLine(D2D1::Point2F(fr - radiuses[1], ft));
                        sink->AddArc(D2D1::ArcSegment(
                            D2D1::Point2F(rect.right - offset, ft + radiuses[1]),
                            D2D1::SizeF((FLOAT)radiuses[1],(FLOAT)radiuses[1]),
                            0.f,D2D1_SWEEP_DIRECTION_CLOCKWISE,D2D1_ARC_SIZE_SMALL));
                    } else {
                        sink->AddLine(D2D1::Point2F(rect.right,ft));
                    }
                    sink->EndFigure(D2D1_FIGURE_END_OPEN);
                    sink->Close();
                    rt->DrawGeometry(pathGeo,brushes[0],(FLOAT)widths.top);
                    SafeRelease(sink);
                    SafeRelease(pathGeo);
                } else {
                    rt->DrawLine(D2D1::Point2F(rect.left,ft), D2D1::Point2F(rect.right,ft),
                        brushes[0],(FLOAT)widths.top);
                }
            }

            if(widths.right > 0) {
                rt->DrawLine(D2D1::Point2F(fr, ft + radiuses[1]), D2D1::Point2F(fr,fb - radiuses[2]),
                    brushes[1],(FLOAT)widths.right);
            }

            if(widths.bottom > 0)
            {
                FLOAT offset = (FLOAT)widths.bottom / 2;
                if(radiuses[2] != 0 || radiuses[3] != 0)
                {
                    ID2D1PathGeometry* pathGeo;
                    ID2D1GeometrySink* sink;
                    mApp->getD2D1Factory()->CreatePathGeometry(&pathGeo);
                    pathGeo->Open(&sink);
                    if(radiuses[2] != 0) {
                        sink->BeginFigure(D2D1::Point2F(fr, fb - radiuses[2]),D2D1_FIGURE_BEGIN_FILLED);
                        sink->AddArc(D2D1::ArcSegment(
                            D2D1::Point2F(rect.right - radiuses[2] - offset,fb),
                            D2D1::SizeF((FLOAT)radiuses[2],(FLOAT)radiuses[2]),
                            0.f,D2D1_SWEEP_DIRECTION_CLOCKWISE,D2D1_ARC_SIZE_SMALL));
                    } else {
                        sink->BeginFigure(D2D1::Point2F(rect.right,fb),D2D1_FIGURE_BEGIN_FILLED);
                    }

                    if(radiuses[3] != 0) {
                        sink->AddLine(D2D1::Point2F(fl + radiuses[3], fb));
                        sink->AddArc(D2D1::ArcSegment(
                            D2D1::Point2F(rect.left + offset, fb - radiuses[3]),
                            D2D1::SizeF((FLOAT)radiuses[3],(FLOAT)radiuses[3]),
                            0.f,D2D1_SWEEP_DIRECTION_CLOCKWISE,D2D1_ARC_SIZE_SMALL));
                    } else {
                        sink->AddLine(D2D1::Point2F(rect.left, fb));
                    }
                    sink->EndFigure(D2D1_FIGURE_END_OPEN);
                    sink->Close();
                    rt->DrawGeometry(pathGeo,brushes[2],(FLOAT)widths.bottom);
                    SafeRelease(sink);
                    SafeRelease(pathGeo);
                } else {
                    rt->DrawLine(D2D1::Point2F(rect.right,fb), D2D1::Point2F(rect.left,fb),
                        brushes[2],(FLOAT)widths.bottom);
                }
            }

            if(widths.left > 0) {
                rt->DrawLine(D2D1::Point2F(fl, fb-radiuses[3]), D2D1::Point2F(fl,ft + radiuses[0]),
                    brushes[3],(FLOAT)widths.left);
            }
        }

        unsigned int TextRenderObject::getGDITextFormat()
        {
            unsigned int formatParam = 0;

            if(testValue(this, Value_Wrap))
                formatParam = DT_WORDBREAK;
            else if(testValue(this, Value_Ellipsis))
                formatParam = DT_END_ELLIPSIS;
            else
                formatParam = DT_SINGLELINE;

            if(testValue(this, Value_HCenter))
                formatParam |= DT_CENTER;
            else if(testValue(this, Value_Right))
                formatParam |= DT_RIGHT;
            else
                formatParam |= DT_LEFT;

            if(testValue(this, Value_Top))
                formatParam |= DT_TOP;
            else if(testValue(this, Value_Bottom))
                formatParam |= DT_BOTTOM;
            else
                formatParam |= DT_VCENTER;
            return formatParam;
        }

        IDWriteTextFormat* TextRenderObject::createDWTextFormat()
        {
            IDWriteTextFormat* tf;
            mApp->getDWriteFactory()->CreateTextFormat(font.getFaceName().c_str(),0,
                font.isBold()   ? DWRITE_FONT_WEIGHT_BOLD  : DWRITE_FONT_WEIGHT_NORMAL,
                font.isItalic() ? DWRITE_FONT_STYLE_ITALIC : DWRITE_FONT_STYLE_NORMAL,
                DWRITE_FONT_STRETCH_NORMAL,
                96.f * font.pointSize() / 72.f, // 12 points = 1/6 logical inch = 96/6 DIPs (96 DIPs = 1 ich)
                L"en-US", &tf); // Remark: don't know what the locale does.

            if(testValue(this, Value_HCenter))
                tf->SetTextAlignment(DWRITE_TEXT_ALIGNMENT_CENTER);
            else if(testValue(this, Value_Right))
                tf->SetTextAlignment(DWRITE_TEXT_ALIGNMENT_TRAILING);
            else
                tf->SetTextAlignment(DWRITE_TEXT_ALIGNMENT_LEADING);

            if(testValue(this, Value_Top))
                tf->SetParagraphAlignment(DWRITE_PARAGRAPH_ALIGNMENT_NEAR);
            else if(testValue(this, Value_Bottom))
                tf->SetParagraphAlignment(DWRITE_PARAGRAPH_ALIGNMENT_FAR);
            else
                tf->SetParagraphAlignment(DWRITE_PARAGRAPH_ALIGNMENT_CENTER);

            return tf;
        }

    } // namespace CSS


    using namespace CSS;
    typedef multimap<PropertyType, Declaration*> DeclMap;

    // ********** RenderRuleData Impl
    ID2D1RenderTarget* RenderRuleData::workingRT = 0;
    RenderRuleData::~RenderRuleData()
    {
        for(int i = backgroundROs.size() -1; i >=0; --i)
            delete backgroundROs.at(i);
        delete borderImageRO;
        delete borderRO;
        delete geoRO;
        delete cursor;
        delete textRO;
        delete margin;
        delete padding;
    }

    MSize RenderRuleData::getStringSize(const std::wstring& s, int maxWidth)
    {
        MSize size;

        MFont defaultFont;
        TextRenderObject defaultRO(defaultFont);
        TextRenderObject* tro = textRO == 0 ? &defaultRO : textRO;

        bool useGDI = false;
        if(RenderRule::getTextRenderer() == AutoDetermine)
        {
            unsigned int fontPtSize = tro->font.pointSize();
            if(fontPtSize <= RenderRule::getMaxGdiFontPtSize())
                useGDI = true;
        }

        if(useGDI || RenderRule::getTextRenderer() == Gdi)
        {
            HDC calcDC = ::GetDC(NULL);
            HFONT oldFont = (HFONT)::SelectObject(calcDC, tro->font.getHandle());

            unsigned int formatParam = DT_CALCRECT;
            if(tro != 0) formatParam |= tro->getGDITextFormat();

            MRect textRect;
            textRect.right = maxWidth;

            size.cy = ::DrawTextW(calcDC, s.c_str(), s.size(), &textRect, formatParam);
            size.cx = textRect.right;

            ::SelectObject(calcDC, oldFont);
            ::ReleaseDC(NULL, calcDC);
        } else
        {
            IDWriteTextFormat* textFormat = tro->createDWTextFormat();
            IDWriteTextLayout* textLayout;
            mApp->getDWriteFactory()->CreateTextLayout(s.c_str(), s.size(),
                textFormat, (FLOAT)maxWidth, 0.f, &textLayout);

            DWRITE_TEXT_METRICS metrics;
            textLayout->GetMetrics(&metrics);

            size.cx = (long)metrics.widthIncludingTrailingWhitespace;
            size.cy = (long)metrics.height;

            SafeRelease(textFormat);
            SafeRelease(textLayout);
        }

        return size;
    }

    MRect RenderRuleData::getContentMargin()
    {
        MRect r;
        if(hasMargin())
        {
            r.left   = margin->left;
            r.right  = margin->right;
            r.top    = margin->top;
            r.bottom = margin->bottom;
        }

        if(hasPadding())
        {
            r.left   += padding->left;
            r.right  += padding->right;
            r.top    += padding->top;
            r.bottom += padding->bottom;
        }

        if(borderRO)
        {
            if(borderRO->type == BorderRenderObject::ComplexBorder)
            {
                ComplexBorderRenderObject* cbro = reinterpret_cast<ComplexBorderRenderObject*>(borderRO);
                r.left   += cbro->widths.left;
                r.right  += cbro->widths.right;
                r.top    += cbro->widths.top;
                r.bottom += cbro->widths.bottom;
            } else
            {
                SimpleBorderRenderObject* sbro = reinterpret_cast<SimpleBorderRenderObject*>(borderRO);
                r.left   += sbro->width;
                r.right  += sbro->width;
                r.top    += sbro->width;
                r.bottom += sbro->width;
            }
        }
        return r;
    }

    bool RenderRuleData::setGeometry(MWidget* w)
    {
        if(geoRO == 0)
            return false;

        if(geoRO->x != INT_MAX)
            w->move(geoRO->x, geoRO->y == INT_MAX ? w->y() : geoRO->y);
        else if(geoRO->y != INT_MAX)
            w->move(w->x(), geoRO->y);

        MSize size  = w->size();
        MSize size2 = size;
        if(geoRO->width  != -1 && geoRO->width  != size2.getWidth())
            size2.width() = geoRO->width;
        if(geoRO->height != -1 && geoRO->height != size2.getHeight())
            size2.height() = geoRO->height;

        MSize range  = w->minSize();
        bool changed = false;
        if(geoRO->minWidth  != -1) { range.width()  = geoRO->minWidth;  changed = true; }
        if(geoRO->minHeight != -1) { range.height() = geoRO->minHeight; changed = true; }
        if(changed) w->setMinimumSize(range.getWidth(),range.getHeight());

        if(size2.width()  < range.width())  size2.width()  = range.width();
        if(size2.height() < range.height()) size2.height() = range.height();

        range   = w->maxSize();
        changed = false;
        if(geoRO->maxWidth  != -1) { range.width()  = geoRO->maxWidth;  changed = true; }
        if(geoRO->maxHeight != -1) { range.height() = geoRO->maxHeight; changed = true; }
        if(changed) w->setMaximumSize(range.width(), range.height());

        if(size2.width()  > range.width())  size2.width()  = range.width();
        if(size2.height() > range.height()) size2.height() = range.height();

        if(size2.width() == size.width() && size2.height() == size.height())
            return false;

        w->resize(size2.width(),size2.height());
        return true;
    }

    void RenderRuleData::draw(MD2DPaintContext& context, const MRect& widgetRectInRT,
        const MRect& clipRectInRT, const wstring& text, unsigned int frameIndex)
    {
        workingRT = context.getRenderTarget();
        M_ASSERT(workingRT != 0);

        D2D1_RECT_F borderRect = widgetRectInRT;
        if(hasMargin()) {
            borderRect.left   += (FLOAT)margin->left;
            borderRect.right  -= (FLOAT)margin->right;
            borderRect.top    += (FLOAT)margin->top;
            borderRect.bottom -= (FLOAT)margin->bottom;
        }

        // We have to get the border geometry first if there's any.
        // Because drawing the background may depends on the border geometry.
        ID2D1Geometry* borderGeo = 0;
        if(borderRO != 0 && borderRO->type == BorderRenderObject::ComplexBorder)
            borderGeo = reinterpret_cast<ComplexBorderRenderObject*>(borderRO)->createGeometry(borderRect);

        // === Backgrounds ===
        if(backgroundROs.size() > 0)
            drawBackgrounds(widgetRectInRT, clipRectInRT, borderGeo, frameIndex);

        // === BorderImage ===
        if(borderImageRO != 0)
        {
            context.fill9PatchRect(borderImageRO->brush,
                testNoValue(borderImageRO, Value_RepeatX),
                testNoValue(borderImageRO, Value_RepeatY),
                widgetRectInRT,
                &clipRectInRT);
        }

        // === Border ===
        if(borderRO != 0)
        {
            if(borderRO->type == BorderRenderObject::ComplexBorder)
            {
                // Check the ComplexBorderRO's brushes here, so that it don't depend on
                // RenderRuleData.
                ComplexBorderRenderObject* cbro = reinterpret_cast<ComplexBorderRenderObject*>(borderRO);
                if(borderRO->isVisible())
                    cbro->draw(workingRT, borderGeo, borderRect);

                borderRect.left  += cbro->widths.left;
                borderRect.top   += cbro->widths.top;
                borderRect.right -= cbro->widths.right;
                borderRect.bottom-= cbro->widths.bottom;
            } else {
                SimpleBorderRenderObject* sbro = reinterpret_cast<SimpleBorderRenderObject*>(borderRO);
                FLOAT w = (FLOAT)sbro->width;
                if(borderRO->isVisible())
                {
                    // MS said that the rectangle stroke is centered on the outline. So we
                    // need to shrink a half of the borderWidth.
                    FLOAT shrink = w / 2;
                    borderRect.left   += shrink;
                    borderRect.top    += shrink;
                    borderRect.right  -= shrink;
                    borderRect.bottom -= shrink;
                    if(borderRO->type == BorderRenderObject::SimpleBorder)
                        workingRT->DrawRectangle(borderRect, sbro->brush, w);
                    else {
                        RadiusBorderRenderObject* rbro = reinterpret_cast<RadiusBorderRenderObject*>(borderRO);
                        D2D1_ROUNDED_RECT rr;
                        rr.rect = borderRect;
                        rr.radiusX = rr.radiusY = (FLOAT)rbro->radius;
                        workingRT->DrawRoundedRectangle(rr, sbro->brush, w);
                    }
                    borderRect.left   += shrink;
                    borderRect.top    += shrink;
                    borderRect.right  -= shrink;
                    borderRect.bottom -= shrink;
                } else
                {
                    borderRect.left   += w;
                    borderRect.right  -= w;
                    borderRect.top    += w;
                    borderRect.bottom -= w;
                }
            }
        }

        // === Text ===
        if(!text.empty())
        {
            if(hasPadding())
            {
                borderRect.left   += padding->left;
                borderRect.top    += padding->top;
                borderRect.right  -= padding->right;
                borderRect.bottom -= padding->bottom;
            }

            switch(RenderRule::getTextRenderer())
            {
            case Gdi:
                drawGdiText(borderRect,clipRectInRT,text);
                break;
            case Direct2D:
                drawD2DText(borderRect,text);
                break;
            case AutoDetermine:
                unsigned int fontPtSize = 12;
                if(textRO != 0) fontPtSize = textRO->font.pointSize();
                if(fontPtSize <= RenderRule::getMaxGdiFontPtSize())
                    drawGdiText(borderRect,clipRectInRT,text);
                else
                    drawD2DText(borderRect,text);
                break;
            }
        }

        SafeRelease(borderGeo);
        workingRT = 0;
    }

    void RenderRuleData::drawBackgrounds(const MRect& wr, const MRect& cr,
        ID2D1Geometry* borderGeo, unsigned int frameIndex)
    {
        for(unsigned int i = 0; i < backgroundROs.size(); ++i)
        {
            MRect contentRect = wr;

            BackgroundRenderObject* bgro  = backgroundROs.at(i);
            if(frameIndex >= bgro->frameCount)
                frameIndex = frameIndex % bgro->frameCount;

            if(testNoValue(bgro,Value_Margin))
            {
                if(hasMargin()) {
                    contentRect.left   += margin->left;
                    contentRect.top    += margin->top;
                    contentRect.right  -= margin->right;
                    contentRect.bottom -= margin->bottom;
                }

                if(testNoValue(bgro,Value_Border)) {
                    if(borderRO != 0) {
                        MRect borderWidth;
                        borderRO->getBorderWidth(borderWidth);
                        contentRect.left   += borderWidth.left;
                        contentRect.top    += borderWidth.top;
                        contentRect.right  -= borderWidth.right;
                        contentRect.bottom -= borderWidth.bottom;
                    }
                    if(hasPadding()) {
                        contentRect.left   += padding->left;
                        contentRect.top    += padding->top;
                        contentRect.right  -= padding->right;
                        contentRect.bottom -= padding->bottom;
                    }
                }
            }

            ID2D1Brush* brush = bgro->brush;
            if(brush != bgro->checkedBrush)
            {
                bgro->checkedBrush = brush;
                // Check the size.
                if(bgro->brush.type() == MD2DBrushHandle::Bitmap)
                {
                    bgro->frameCount   = bgro->brush.frameCount();

                    ID2D1BitmapBrush* bb = (ID2D1BitmapBrush*)brush;
                    ID2D1Bitmap* bmp;
                    bb->GetBitmap(&bmp);
                    D2D1_SIZE_F size = bmp->GetSize();
                    unsigned int& w = bgro->width;
                    unsigned int& h = bgro->height;
                    // Make sure the size is valid.
                    if(w == 0 || w + bgro->x > (unsigned int)size.width )
                        w = (unsigned int)size.width  - bgro->x;
                    if(h == 0 || h + bgro->y > (unsigned int)size.height)
                        h = (unsigned int)size.height - bgro->y;
                    h /= bgro->frameCount;

                    if(bgro->frameCount != 1)
                    {
                        if(totalFrameCount % bgro->frameCount != 0)
                            totalFrameCount *= bgro->frameCount;
                        bgro->values &= (~Value_Repeat);
                    }
                }
            }

            int dx = 0 - bgro->x;
            int dy = 0 - bgro->y;
            if(bgro->width != 0)
            {
                if(testNoValue(bgro,Value_Left)) {
                    if(testValue(bgro,Value_HCenter))
                        contentRect.left = (contentRect.left + contentRect.right - bgro->width) / 2;
                    else
                        contentRect.left = contentRect.right - bgro->width;
                }
                if(testNoValue(bgro,Value_RepeatX)) {
                    int r = contentRect.left + bgro->width;
                    if(contentRect.right > r)
                        contentRect.right  = r;
                }
            }
            if(bgro->height != 0)
            {
                if(testNoValue(bgro, Value_Top)) {
                    if(testValue(bgro, Value_VCenter))
                        contentRect.top = (contentRect.top + contentRect.bottom - bgro->height) / 2;
                    else
                        contentRect.top = contentRect.bottom - bgro->height;
                }
                if(testNoValue(bgro, Value_RepeatY)) {
                    int b = contentRect.top + bgro->height;
                    if(contentRect.bottom > b)
                        contentRect.bottom = b;
                }
            }

            dx += (int)contentRect.left;
            dy += (int)contentRect.top;
            dy -= frameIndex * bgro->height;
            if(dx != 0 || dy != 0)
                brush->SetTransform(D2D1::Matrix3x2F::Translation((FLOAT)dx,(FLOAT)dy));

            D2D1_RECT_F clip;
            clip.left   = (FLOAT) max(contentRect.left  ,cr.left);
            clip.top    = (FLOAT) max(contentRect.top   ,cr.top);
            clip.right  = (FLOAT) min(contentRect.right ,cr.right);
            clip.bottom = (FLOAT) min(contentRect.bottom,cr.bottom);

            if(bgro->brush.type() == MD2DBrushHandle::LinearGradient)
            {
                D2D1_RECT_F cr = contentRect;
                bgro->brush.getLinearGraidentBrush(&cr);
            }

            if(testNoValue(bgro,Value_Border) || borderRO == 0)
                workingRT->FillRectangle(clip,brush);
            else
            {
                if(borderRO->type == BorderRenderObject::SimpleBorder) {
                    workingRT->FillRectangle(clip,brush);
                } else if(borderRO->type == BorderRenderObject::RadiusBorder) {
                    D2D1_ROUNDED_RECT rrect;
                    rrect.rect    = clip;
                    rrect.radiusX = FLOAT(reinterpret_cast<RadiusBorderRenderObject*>(borderRO)->radius);
                    rrect.radiusY = rrect.radiusX;
                    workingRT->FillRoundedRectangle(rrect,brush);
                } else {
                    workingRT->FillGeometry(borderGeo,brush);
                }
            }
        }
    }

    class D2DOutlineTextRenderer : public IDWriteTextRenderer
    {
        public:
            D2DOutlineTextRenderer(ID2D1RenderTarget* r, TextRenderObject* tro, ID2D1Brush* outlineBrush,
                ID2D1Brush* textBrush, ID2D1Brush* shadowBrush = 0):
            rt(r),tRO(tro),obrush(outlineBrush),tbrush(textBrush),sbrush(shadowBrush){}

            HRESULT __stdcall DrawGlyphRun(void* clientDrawingContext, FLOAT baselineOriginX, FLOAT baselineOriginY,
                DWRITE_MEASURING_MODE measuringMode, DWRITE_GLYPH_RUN const* glyphRun,
                DWRITE_GLYPH_RUN_DESCRIPTION const* glyphRunDescription,
                IUnknown* clientDrawingEffect);

            HRESULT __stdcall IsPixelSnappingDisabled(void*, BOOL* isDiabled) { *isDiabled = FALSE; return S_OK; }
            HRESULT __stdcall GetCurrentTransform(void*, DWRITE_MATRIX* matrix) { rt->GetTransform(reinterpret_cast<D2D1_MATRIX_3X2_F*>(matrix)); return S_OK; }
            HRESULT __stdcall GetPixelsPerDip(void*, FLOAT* ppd) { *ppd = 1.f; return S_OK; }

            HRESULT __stdcall DrawUnderline(void*,FLOAT,FLOAT,DWRITE_UNDERLINE const*,IUnknown*) { return E_NOTIMPL; }
            HRESULT __stdcall DrawStrikethrough(void*,FLOAT,FLOAT,DWRITE_STRIKETHROUGH const*,IUnknown*) { return E_NOTIMPL; }
            HRESULT __stdcall DrawInlineObject(void*,FLOAT,FLOAT,IDWriteInlineObject*,BOOL,BOOL,IUnknown*) { return E_NOTIMPL; }
            unsigned long __stdcall AddRef() { return 0; }
            unsigned long __stdcall Release(){ return 0; }
            HRESULT __stdcall QueryInterface(IID const&, void**) { return E_NOTIMPL; }

        private:
            ID2D1RenderTarget* rt;
            ID2D1Brush* obrush;
            ID2D1Brush* tbrush;
            ID2D1Brush* sbrush;
            TextRenderObject* tRO;
    };

    HRESULT D2DOutlineTextRenderer::DrawGlyphRun(void*, FLOAT baselineOriginX, FLOAT baselineOriginY,
        DWRITE_MEASURING_MODE, DWRITE_GLYPH_RUN const* glyphRun, DWRITE_GLYPH_RUN_DESCRIPTION const*, IUnknown*)
    {
        ID2D1PathGeometry* pPathGeometry = NULL;
        ID2D1GeometrySink* pSink = NULL;
        mApp->getD2D1Factory()->CreatePathGeometry(&pPathGeometry); 
        pPathGeometry->Open(&pSink);
        glyphRun->fontFace->GetGlyphRunOutline(
            glyphRun->fontEmSize,
            glyphRun->glyphIndices,
            glyphRun->glyphAdvances,
            glyphRun->glyphOffsets,
            glyphRun->glyphCount,
            glyphRun->isSideways,
            glyphRun->bidiLevel%2,
            pSink);

        pSink->Close();

        // Initialize a matrix to translate the origin of the glyph run.
        D2D1::Matrix3x2F const matrix = D2D1::Matrix3x2F(
            1.0f, 0.0f, 0.0f, 1.0f,
            baselineOriginX, baselineOriginY);

        ID2D1TransformedGeometry* pTransformedGeometry = NULL;
        mApp->getD2D1Factory()->CreateTransformedGeometry(pPathGeometry, &matrix, &pTransformedGeometry);
        ID2D1StrokeStyle* pStrokeStyle;
        D2D1_STROKE_STYLE_PROPERTIES strokeStyle = D2D1::StrokeStyleProperties(
            D2D1_CAP_STYLE_FLAT,D2D1_CAP_STYLE_FLAT,D2D1_CAP_STYLE_FLAT,D2D1_LINE_JOIN_ROUND);
        mApp->getD2D1Factory()->CreateStrokeStyle(&strokeStyle,0,0,&pStrokeStyle);
        if(sbrush != 0)
        {
            // Draw shadow.
            // Remark: If the shadow color is not opaque, we will see the outline of the shadow.
            // We can however render the opaque shadow to a bitmap then render the bitmap to the
            // rendertarget with opacity applied.
            ID2D1TransformedGeometry* shadowGeo = NULL;
            D2D1::Matrix3x2F const shadowMatrix = D2D1::Matrix3x2F(
                1.0f, 0.0f, 0.0f, 1.0f,
                baselineOriginX + tRO->shadowOffsetX, baselineOriginY + tRO->shadowOffsetY);
            mApp->getD2D1Factory()->CreateTransformedGeometry(pPathGeometry, &shadowMatrix, &shadowGeo);

            rt->DrawGeometry(shadowGeo,sbrush,tRO->outlineWidth,pStrokeStyle);
            rt->FillGeometry(shadowGeo,sbrush);

            SafeRelease(shadowGeo);
        }

        rt->DrawGeometry(pTransformedGeometry,obrush,tRO->outlineWidth,pStrokeStyle);
        rt->FillGeometry(pTransformedGeometry,tbrush);

        SafeRelease(pStrokeStyle);
        SafeRelease(pPathGeometry);
        SafeRelease(pSink);
        SafeRelease(pTransformedGeometry);
        return S_OK;
    }

    void RenderRuleData::drawD2DText(const D2D1_RECT_F& drawRect, const wstring& text)
    {
        MFont defaultFont;
        TextRenderObject defaultRO(defaultFont);
        TextRenderObject* tro = textRO == 0 ? &defaultRO : textRO;
        bool hasOutline = tro->outlineWidth != 0 && !tro->outlineColor.isTransparent();
        bool hasShadow  = !tro->shadowColor.isTransparent() &&
            ( tro->shadowBlur != 0 || (tro->shadowOffsetX != 0 || tro->shadowOffsetY != 0) );

        IDWriteTextFormat* textFormat = tro->createDWTextFormat();
        IDWriteTextLayout* textLayout = 0;
        mApp->getDWriteFactory()->CreateTextLayout(text.c_str(), text.size(), textFormat,
            drawRect.right - drawRect.left, drawRect.bottom - drawRect.top, &textLayout);

        if(hasOutline)
        {
            ID2D1Brush* sbrush = 0;
            if(hasShadow) sbrush = tro->shadowBrush;
            D2DOutlineTextRenderer renderer(workingRT,textRO,tro->outlineBrush,tro->textBrush,sbrush);
            textLayout->Draw(0,&renderer,drawRect.left,drawRect.top);
        } else
        {
            if(hasShadow)
            {
                workingRT->DrawTextLayout(
                    D2D1::Point2F(drawRect.left + tro->shadowOffsetX, drawRect.top + tro->shadowOffsetY),
                    textLayout, tro->shadowBrush, D2D1_DRAW_TEXT_OPTIONS_CLIP);
            }
            workingRT->DrawTextLayout(D2D1::Point2F(drawRect.left, drawRect.top),
                textLayout, tro->textBrush, D2D1_DRAW_TEXT_OPTIONS_CLIP);
        }

        SafeRelease(textFormat);
        SafeRelease(textLayout);
    }

#ifndef MB_NO_GDIPLUS
    using namespace Gdiplus;
#endif
    void RenderRuleData::drawGdiText(const D2D1_RECT_F& borderRect,
        const MRect& clipRectInRT, const wstring& text)
    {
        // TODO: Add support for underline and its line style.

        HRESULT result = workingRT->Flush();
        if(result != S_OK) return;

        HDC textDC;
        ID2D1GdiInteropRenderTarget* gdiTarget;
        workingRT->QueryInterface(&gdiTarget);
        result = gdiTarget->GetDC(D2D1_DC_INITIALIZE_MODE_COPY,&textDC);

        bool hasClip = false;
        if(result == D2DERR_RENDER_TARGET_HAS_LAYER_OR_CLIPRECT)
        {
            hasClip = true;
            workingRT->PopAxisAlignedClip();
            gdiTarget->GetDC(D2D1_DC_INITIALIZE_MODE_COPY,&textDC);
        }

        MFont defaultFont;
        TextRenderObject defaultRO(defaultFont);
        TextRenderObject* tro = textRO == 0 ? &defaultRO : textRO;
        bool hasOutline = tro->outlineWidth != 0 && !tro->outlineColor.isTransparent();
        bool hasShadow  = !tro->shadowColor.isTransparent() &&
            ( tro->shadowBlur != 0 || (tro->shadowOffsetX != 0 || tro->shadowOffsetY != 0) );

        MRect drawRect((LONG)borderRect.left, (LONG)borderRect.top,
            (LONG)borderRect.right, (LONG)borderRect.bottom);

        HRGN  clipRGN = ::CreateRectRgn(drawRect.left, drawRect.top, drawRect.right, drawRect.bottom);
        HRGN  oldRgn  = (HRGN)::SelectObject(textDC,clipRGN);
        HFONT oldFont = (HFONT)::SelectObject(textDC,tro->font.getHandle());
        ::SetBkMode(textDC,TRANSPARENT);

#ifndef MB_NO_GDIPLUS
        // Draw the text
        if(hasOutline) // If we need to draw the outline, we have to use GDI+.
        {
            StringFormat sf;
            if(testValue(textRO, Value_Ellipsis))
                sf.SetTrimming(StringTrimmingEllipsisWord);
            else
                sf.SetTrimming(StringTrimmingNone);
            if(testValue(textRO, Value_Wrap))
                sf.SetFormatFlags(StringFormatFlagsNoWrap);
            if(testValue(textRO, Value_HCenter))
                sf.SetAlignment(StringAlignmentCenter);
            else if(testValue(textRO, Value_Right))
                sf.SetAlignment(StringAlignmentFar);
            else
                sf.SetAlignment(StringAlignmentNear);
            if(testValue(textRO, Value_Top))
                sf.SetLineAlignment(StringAlignmentNear);
            else if(testValue(textRO, Value_Bottom))
                sf.SetLineAlignment(StringAlignmentFar);
            else
                sf.SetLineAlignment(StringAlignmentCenter);

            GraphicsPath textPath;
            Font f(textDC);
            FontFamily fm;
            f.GetFamily(&fm);
            unsigned int style = FontStyleRegular;
            if(tro->font.isBold())
                style = tro->font.isItalic() ? FontStyleBoldItalic : FontStyleBold;
            else if(tro->font.isItalic())
                style = FontStyleItalic;
            int w = drawRect.width() + tro->shadowBlur * 2;
            int h = drawRect.height()+ tro->shadowBlur * 2;
            Rect layoutRect(drawRect.left + tro->shadowOffsetX, drawRect.top + tro->shadowOffsetY, w, h);
            textPath.AddString(text.c_str(), -1, &fm, style, f.GetSize(), layoutRect, &sf);

            Graphics graphics(textDC);
            graphics.SetSmoothingMode(SmoothingModeAntiAlias);
            if(hasShadow)
            {
                Pen shadowPen(tro->shadowColor.getARGB());
                shadowPen.SetLineJoin(LineJoinRound);
                shadowPen.SetWidth(tro->outlineWidth);
                graphics.DrawPath(&shadowPen,&textPath);
                SolidBrush shadowBrush(tro->shadowColor.getARGB());
                graphics.FillPath(&shadowBrush,&textPath);
            }
            if(tro->shadowOffsetX != 0 || tro->shadowOffsetY != 0)
            {
                Matrix matrix;
                matrix.Translate((REAL)-tro->shadowOffsetX,(REAL)-tro->shadowOffsetY);
                textPath.Transform(&matrix);
            }
            Pen outlinePen(tro->outlineColor.getARGB());
            outlinePen.SetLineJoin(LineJoinRound);
            outlinePen.SetWidth(tro->outlineWidth);
            graphics.DrawPath(&outlinePen,&textPath);
            SolidBrush textBrush(tro->color.getARGB());
            graphics.FillPath(&textBrush,&textPath);
        } else
#endif
        {
            unsigned int formatParam = tro->getGDITextFormat();
            if(hasShadow)
            {
                if(tro->shadowBlur != 0 || tro->shadowColor.getAlpha() != 0xFF)
                {
                    HDC tempDC = ::CreateCompatibleDC(0);
                    void* pvBits;

                    int width  = drawRect.width() + tro->shadowBlur * 2;
                    int height = drawRect.height()+ tro->shadowBlur * 2;

                    BITMAPINFO bmi = {};
                    bmi.bmiHeader.biSize        = sizeof(BITMAPINFOHEADER);
                    bmi.bmiHeader.biWidth       = width;
                    bmi.bmiHeader.biHeight      = height;
                    bmi.bmiHeader.biCompression = BI_RGB;
                    bmi.bmiHeader.biPlanes      = 1;
                    bmi.bmiHeader.biBitCount    = 32;

                    HBITMAP tempBMP = ::CreateDIBSection(tempDC, &bmi, DIB_RGB_COLORS, &pvBits, 0, 0);
                    HBITMAP oldBMP  = (HBITMAP)::SelectObject(tempDC, tempBMP);
                    HFONT   oldFont = (HFONT)  ::SelectObject(tempDC, tro->font);

                    ::SetTextColor(tempDC, RGB(255,255,255));
                    ::SetBkColor  (tempDC, RGB(0,0,0));
                    ::SetBkMode   (tempDC, OPAQUE);

                    MRect shadowRect(tro->shadowBlur, tro->shadowBlur, drawRect.width(), drawRect.height());
                    ::DrawTextW(tempDC, (LPWSTR)text.c_str(), -1, &shadowRect, formatParam);

                    unsigned int* pixel = (unsigned int*)pvBits;
                    for(int i = width * height - 1; i >= 0; --i)
                    {
                        if(pixel[i] != 0) {
                            if(pixel[i] == 0xFFFFFF)
                                pixel[i] = 0xFF000000 | tro->shadowColor.getRGB();
                            else
                            {
                                BYTE alpha = (pixel[i] >> 24) & 0xFF;
                                BYTE* component = (BYTE*)(pixel+i);
                                unsigned int color = tro->shadowColor.getARGB();
                                component[0] = ( color      & 0xFF) * alpha >> 8; 
                                component[1] = ((color >> 8)& 0xFF) * alpha >> 8;
                                component[2] = ((color >>16)& 0xFF) * alpha >> 8;
                                component[3] = alpha;
                            }
                        }
                    }

                    if(tro->shadowBlur == 0)
                    {
                        BLENDFUNCTION bf = { AC_SRC_OVER, 0, tro->shadowColor.getAlpha(), AC_SRC_ALPHA };
                        int xCor = drawRect.left - tro->shadowBlur;
                        int yCor = drawRect.top  - tro->shadowBlur;
                        int tempXCor = xCor;
                        int tempYCor = yCor;
                        if(xCor < clipRectInRT.left) xCor = clipRectInRT.left;
                        if(yCor < clipRectInRT.top ) yCor = clipRectInRT.top;
                        int w = 0 - xCor + (clipRectInRT.right  < drawRect.right  ? clipRectInRT.right  : drawRect.right);
                        int h = 0 - yCor + (clipRectInRT.bottom < drawRect.bottom ? clipRectInRT.bottom : drawRect.bottom);
                        ::GdiAlphaBlend(textDC, xCor + tro->shadowOffsetX, yCor + tro->shadowOffsetY,
                            w, h, tempDC, xCor - tempXCor, yCor - tempYCor, w, h, bf);
                    } else {
                        // TODO: Implement blur. Don't know why the GDI+ version is still 1.0 on Win7 SDK,
                        // So, we cannot use the gdiplus's blur effect.
                    }

                    ::SelectObject(tempDC,oldBMP);
                    ::SelectObject(tempDC,oldFont);
                    ::DeleteObject(tempBMP);
                    ::DeleteDC(tempDC);

                } else {

                    MRect shadowRect = drawRect;
                    shadowRect.offset(tro->shadowOffsetX,tro->shadowOffsetY);
                    ::SetTextColor(textDC, tro->shadowColor.getCOLORREF());
                    ::DrawTextW(textDC, (LPWSTR)text.c_str(), -1, &shadowRect, formatParam);
                }
            }

            ::SetTextColor(textDC, tro->color.getCOLORREF());
            ::DrawTextW(textDC, (LPWSTR)text.c_str(), -1, &drawRect, formatParam);
        }

        // Remark: Maybe we could make fixAlpha an option;
        // Restrict the fixing area to the Text's bounding rect,
        // not the entire drawing rect.
        bool fixAlpha = true;
        if(fixAlpha)
        {
            HDC tempDC = ::CreateCompatibleDC(0);
            void* pvBits;
            BITMAPINFO bmi = {};
            int width  = drawRect.width();
            int height = drawRect.height();
            bmi.bmiHeader.biSize        = sizeof(BITMAPINFOHEADER);
            bmi.bmiHeader.biWidth       = width;
            bmi.bmiHeader.biHeight      = height;
            bmi.bmiHeader.biCompression = BI_RGB;
            bmi.bmiHeader.biPlanes      = 1;
            bmi.bmiHeader.biBitCount    = 32;
            HBITMAP tempBMP = ::CreateDIBSection(tempDC, &bmi, DIB_RGB_COLORS, &pvBits, 0, 0);
            HBITMAP oldBMP  = (HBITMAP)::SelectObject(tempDC, tempBMP);

            ::BitBlt(tempDC, 0, 0, width, height, textDC, drawRect.left, drawRect.top, SRCCOPY);

            unsigned int* pixel = (unsigned int*)pvBits;
            for(int i = width * height - 1; i >= 0; --i)
            {
                BYTE* component = (BYTE*)(pixel + i);
                if(component[3] == 0)
                    component[3] = 0xFF;
            }

            ::SetDIBitsToDevice(textDC, drawRect.left, drawRect.top,
                width, height, 0, 0, 0, height, pvBits, &bmi, DIB_RGB_COLORS);
            ::SelectObject(tempDC, oldBMP);
            ::DeleteObject(tempBMP);
            ::DeleteDC(tempDC);
        }

        ::SelectObject(textDC,oldFont);
        ::SelectObject(textDC,oldRgn);
        ::DeleteObject(clipRGN);

        gdiTarget->ReleaseDC(0);
        gdiTarget->Release();

        if(hasClip)
            workingRT->PushAxisAlignedClip(clipRectInRT, D2D1_ANTIALIAS_MODE_ALIASED);
    }


    enum BorderType { BT_Simple, BT_Radius, BT_Complex };
    enum BackgroundOpaqueType {
        NonOpaque,
        OpaqueClipMargin,
        OpaqueClipBorder,
        OpaqueClipPadding
    };
    BorderType testBorderObjectType(const DeclMap::iterator& declIter,
        const DeclMap::iterator& declIterEnd)
    {
        DeclMap::iterator seeker = declIter;
        while(seeker != declIterEnd && seeker->first <= PT_BorderRadius)
        {
            switch(seeker->first) {
            case PT_Border:
                {
                    CssValueArray& values = seeker->second->values;
                    // If there're more than 3 values in PT_Border, it must be complex.
                    if(values.size() > 3)
                        return BT_Complex;
                    for(unsigned int i = 1; i < values.size() ; ++i)
                    { // If there are the same type values, it's complex.
                        if(values.at(i - 1).type == values.at(i).type)
                            return BT_Complex;
                    }
                }
                break;
            case PT_BorderWidth:
            case PT_BorderColor:
            case PT_BorderStyles:
                if(seeker->second->values.size() > 1)
                    return BT_Complex;
                break;
            case PT_BorderRadius:	return seeker->second->values.size() > 1 ? BT_Complex : BT_Radius;
            default: return BT_Complex;
            }
            ++seeker;
        }
        return BT_Simple;
    }

    // ********** Create The BackgroundRO
    bool isBrushValueOpaque(const CssValue& value)
    {
        if(value.type == CssValue::Color)
            return value.getColor().getAlpha() >= 0xFF000000;

        if(value.type == CssValue::LiearGradient)
        {
            LinearGradientData* data = value.getLinearGradientData();
            for(int i = 0; i < data->stopCount; ++i)
            {
                if(data->stops[i].argb < 0xFF000000)
                    return false;
            }
            return true;
        }

        if(value.type == CssValue::Uri)
        {
            const wstring* uri = value.data.vstring;
            wstring ex = uri->substr(uri->size() - 3);
            transform(ex.begin(),ex.end(),ex.begin(),::tolower);
            if(ex.find(L"png") == 0 || ex.find(L"gif") == 0)
                return false;
            return true;
        }
        
        return false;
    }
    // Return true if the "prop" affects alignX
    bool setBGROProperty(BackgroundRenderObject* object, ValueType prop, bool isPrevPropAlignX)
    {
        switch(prop) {

        case Value_RepeatX:
        case Value_RepeatY:
        case Value_Repeat:
            object->values |= prop;
            break;

        case Value_Padding:
        case Value_Content:
        case Value_Border:
        case Value_Margin:
            {
                unsigned int mask = Value_Padding | Value_Content | Value_Border | Value_Margin;
                object->values &= (~mask);
                object->values |= prop;
                break;
            }

        case Value_NoRepeat:     object->values &= (~Value_Repeat); break;
        case Value_SingleLoop:   object->infiniteLoop = false; break;

        case Value_Left:
        case Value_Right:
            {
                unsigned int mask = Value_Left | Value_Right | Value_HCenter;
                object->values &= (~mask);
                object->values |= prop;
                return true;
            }
        case Value_Top:
        case Value_Bottom:
            {
                unsigned int mask = Value_Top | Value_Bottom | Value_VCenter;
                object->values &= (~mask);
                object->values |= prop;
                break;
            }
            // We both set alignment X & Y, because one may only specific a alignment value.
        case Value_Center:
            {
                unsigned int mask = Value_Top | Value_Bottom | Value_VCenter;
                object->values &= (~mask);
                object->values |= Value_VCenter;
                if(!isPrevPropAlignX)
                {
                    mask = Value_Left | Value_Right | Value_HCenter;
                    object->values &= (~mask);
                    object->values |= Value_HCenter;
                    return true;
                }
            }
        }
        return false;
    }
    void batchSetBGROProps(vector<BackgroundRenderObject*>& bgros,
        ValueType prop, bool isPrevPropAlignX = false)
    {
        vector<BackgroundRenderObject*>::iterator iter = bgros.begin();
        vector<BackgroundRenderObject*>::iterator iterEnd = bgros.end();
        while(iter != iterEnd) { setBGROProperty(*iter,prop,isPrevPropAlignX); ++iter; }
    }

    // Background CSS Syntax:
    // General -  [ Brush/Image frame-count Repeat Clip Alignment rect(x,y,width,height) Opaque ];
    // Details -
    // 1. The color or image must be the first value.
    // 2. If the second value is Number, then it's considered to be frame-count.
    // 3. Repeat, Clip, Alignment, rect and Opaque can be random order. And they don't have to be
    //    in the declaration.
    // 
    // If we want to make it more like CSS3 syntax,
    // we can parse a CSS3 syntax background into multiple Background Declarations.
    BackgroundRenderObject* RenderRuleData::createBackgroundRO(CssValueArray& values,bool* forceOpaque)
    {
        *forceOpaque = false;
        const CssValue& brushValue = values.at(0);
        BackgroundRenderObject* newObject = new BackgroundRenderObject();
        if(brushValue.type == CssValue::Uri) {
            newObject->brush = MD2DBrushHandle::create(brushValue.getString());
        } else if(brushValue.type == CssValue::LiearGradient) {
            newObject->brush = MD2DBrushHandle::create(brushValue.getLinearGradientData());
        } else{
            newObject->brush = MD2DBrushHandle::create(brushValue.getColor());
        }

        size_t index = 1;
        if(values.size() > 1 && values.at(1).type == CssValue::Number)
        {
            int frameCount = values.at(1).getInt();
            if(frameCount > 1) newObject->frameCount = frameCount;
            ++index;
        }
        size_t valueCount = values.size();
        bool isPropAlignX = false;
        while(index < valueCount)
        {
            const CssValue& v = values.at(index);
            if(v.type == CssValue::Identifier) {
                if(v.getIdentifier() == Value_Opaque)
                    *forceOpaque = true;
                else
                    isPropAlignX = setBGROProperty(newObject, v.getIdentifier(), isPropAlignX);
            } else if(v.type == CssValue::Rectangle) {
                const MRect& rect = v.getRect();
                newObject->x      = rect.left;
                newObject->y      = rect.top;
                newObject->width  = rect.width();
                newObject->height = rect.height();
            }
            ++index;
        }
        return newObject;
    }

    // ********** Create the BorderImageRO
    // BorderImage CSS Syntax:
    // General - [ Image, rect{0,1}, (Repeat,Stretch){0,2}, Opaque, Number{1,4}]
    // rect and repeat/stretch don't have to exist. The order of everything
    // cannot change.
    void RenderRuleData::createBorderImageRO(CssValueArray& values,bool* forceOpaque)
    {
        *forceOpaque = false;
        if(values.size() < 2 || 
            values.at(values.size() - 1).type != CssValue::Number) return;

        int index = values.at(1).type == CssValue::Rectangle ? 2 : 1;

        borderImageRO = new BorderImageRenderObject();

        // Repeat or Stretch
        if(values.at(index).type == CssValue::Identifier)
        {
            if(values.at(index + 1).type == CssValue::Identifier)
            {
                if(values.at(index).getIdentifier() != Value_Stretch)
                    borderImageRO->values |= Value_RepeatX;

                if(values.at(index+1).getIdentifier() != Value_Stretch)
                    borderImageRO->values |= Value_RepeatY;

                index += 2;
            } else {
                if(values.at(index).getIdentifier() != Value_Stretch)
                    borderImageRO->values = Value_Repeat;
                ++index;
            }
        }

        if(values.at(index).type == CssValue::Identifier)
        {
            if(values.at(index).getIdentifier() == Value_Opaque)
                *forceOpaque = true;
            ++index;
        }

        MRectU borderWidth;
        // Border
        if(values.size() - index == 1) {
            borderWidth.left   = borderWidth.right =
                borderWidth.bottom = borderWidth.top   = values.at(index).getUInt();
        } else if(values.size() - index < 4)  {
            borderWidth.bottom = borderWidth.top  = values.at(index).getUInt();
            borderWidth.right  = borderWidth.left = values.at(index+1).getUInt();
        } else
        {
            borderWidth.top    = values.at(index    ).getUInt();
            borderWidth.right  = values.at(index + 1).getUInt();
            borderWidth.bottom = values.at(index + 2).getUInt();
            borderWidth.left   = values.at(index + 3).getUInt();
        }

        if(values.at(1).type == CssValue::Rectangle)
        {
            MRect imageRect = values.at(1).getRect();
            borderImageRO->brush = MD2DBrushHandle::createNinePatch(
                values.at(0).getString(),borderWidth,(MRectU*)&imageRect);
        } else {
            borderImageRO->brush = MD2DBrushHandle::createNinePatch(
                values.at(0).getString(),borderWidth,0);
        }
    }

    // ********** Set the SimpleBorderRO
    void RenderRuleData::setSimpleBorderRO(DeclMap::iterator& iter,
        DeclMap::iterator iterEnd)
    {
        SimpleBorderRenderObject* obj = reinterpret_cast<SimpleBorderRenderObject*>(borderRO);
        CssValue colorValue(CssValue::Color);
        LinearGradientData* lgData = 0;
        while(iter != iterEnd)
        {
            vector<CssValue>& values = iter->second->values;
            switch(iter->first) {
            case PT_Border:
                for(unsigned int i = 0; i < values.size(); ++i) {
                    CssValue& v = values.at(i);
                    switch(v.type) {
                    case CssValue::Identifier: obj->style = v.getIdentifier(); break;
                    case CssValue::Color:      colorValue = v;                 break;
                    case CssValue::Length:
                    case CssValue::Number:     obj->width = v.getUInt();       break;
                    case CssValue::LiearGradient: lgData  = v.getLinearGradientData(); break;
                    }
                }
                break;
            case PT_BorderWidth:  obj->width = values.at(0).getUInt();       break;
            case PT_BorderStyles: obj->style = values.at(0).getIdentifier(); break;
            case PT_BorderColor:  
                if(values.at(0).type == CssValue::LiearGradient)
                    lgData = values.at(0).getLinearGradientData();
                else
                    colorValue = values.at(0);
                break;
            }

            ++iter;
        }

        if(lgData == 0)
        {
            MColor color(colorValue.getColor());
            obj->isColorTransparent = color.isTransparent();
            obj->brush = MD2DBrushHandle::create(color);
        } else
        {
            obj->isColorTransparent = false;
            obj->brush = MD2DBrushHandle::create(lgData);
        }
    }

    // ********** Create the ComplexBorderRO
    void setGroupUintValue(unsigned int (&intArray)[4], vector<CssValue>& values,
        int startValueIndex = 0, int endValueIndex = -1)
    {
        int size = (endValueIndex == -1 ? values.size() : endValueIndex + 1) - startValueIndex;
        if(size == 4) {
            intArray[0] = values.at(startValueIndex    ).getUInt();
            intArray[1] = values.at(startValueIndex + 1).getUInt();
            intArray[2] = values.at(startValueIndex + 2).getUInt();
            intArray[3] = values.at(startValueIndex + 3).getUInt();
        } else if(size == 2) {
            intArray[0] = intArray[2] = values.at(startValueIndex    ).getUInt();
            intArray[1] = intArray[3] = values.at(startValueIndex + 1).getUInt();
        } else {
            intArray[0] = intArray[1] =
                intArray[2] = intArray[3] = values.at(startValueIndex).getUInt();
        }
    }
    void setRectUValue(MRectU& rect, vector<CssValue>& values,
        int startValueIndex = 0, int endValueIndex = -1)
    {
        int size = (endValueIndex == -1 ? values.size() : endValueIndex + 1) - startValueIndex;
        if(size == 4) {
            rect.top    = values.at(startValueIndex    ).getUInt();
            rect.right  = values.at(startValueIndex + 1).getUInt();
            rect.bottom = values.at(startValueIndex + 2).getUInt();
            rect.left   = values.at(startValueIndex + 3).getUInt();
        } else if(size == 2) {
            rect.top    = rect.bottom = values.at(startValueIndex    ).getUInt();
            rect.right  = rect.left   = values.at(startValueIndex + 1).getUInt();
        } else {
            rect.top    = rect.bottom =
                rect.right  = rect.left   = values.at(startValueIndex).getUInt();
        }
    }
    void setRectUValue(MRectU& rect, int border, unsigned int value)
    {
        if(border == 0) { rect.top    = value; return; }
        if(border == 1) { rect.right  = value; return; }
        if(border == 2) { rect.bottom = value; return; }
        rect.left = value;
    }
    void RenderRuleData::setComplexBorderRO(DeclMap::iterator& declIter,
        DeclMap::iterator declIterEnd)
    {
        ComplexBorderRenderObject* obj = reinterpret_cast<ComplexBorderRenderObject*>(borderRO);

        MColor colors[4]; // T, R, B, L
        for(int i = 0; i < 4; ++i)
            colors[i].setAlpha(0);

        while(declIter != declIterEnd)
        {
            vector<CssValue>& values = declIter->second->values;
            switch(declIter->first)
            {
            case PT_Border:
                {
                    int rangeStartIndex = 0;
                    CssValue::Type valueType = values.at(0).type;
                    unsigned int index = 1;
                    while(index <= values.size())
                    {
                        if(index == values.size() || values.at(index).type != valueType)
                        {
                            switch(valueType) {
                            case CssValue::Identifier:
                                setRectUValue(obj->styles,values,rangeStartIndex,index - 1);
                                break;
                            case CssValue::Color:
                                {
                                    int size = index - rangeStartIndex;
                                    if(size  == 4) {
                                        colors[0]  = MColor(values.at(rangeStartIndex  ).getUInt());
                                        colors[1]  = MColor(values.at(rangeStartIndex+1).getUInt());
                                        colors[2]  = MColor(values.at(rangeStartIndex+2).getUInt());
                                        colors[3]  = MColor(values.at(rangeStartIndex+3).getUInt());
                                    } else if(size == 2) {
                                        colors[0]  = colors[2]  = MColor(values.at(rangeStartIndex  ).getUInt());
                                        colors[1]  = colors[3]  = MColor(values.at(rangeStartIndex+1).getUInt());
                                    } else {
                                        colors[0]  = colors[2]  =
                                            colors[1]  = colors[3]  = MColor(values.at(rangeStartIndex).getUInt());
                                    }
                                }
                                break;
                            default:
                                setRectUValue(obj->widths,values,rangeStartIndex,index-1);
                            }
                            rangeStartIndex = index;
                            if(index < values.size())
                                valueType = values.at(index).type;
                        }
                        ++index;
                    }
                }
                break;

            case PT_BorderRadius:  setGroupUintValue(obj->radiuses, values); break;
            case PT_BorderWidth:   setRectUValue(obj->widths, values);       break;
            case PT_BorderStyles:  setRectUValue(obj->styles, values);       break;
            case PT_BorderColor:
                {
                    if(values.size() == 4) {
                        colors[0]  = MColor(values.at(0).getUInt());
                        colors[1]  = MColor(values.at(1).getUInt());
                        colors[2]  = MColor(values.at(2).getUInt());
                        colors[3]  = MColor(values.at(3).getUInt());
                    } else if(values.size() == 2) {
                        colors[0]  = colors[2]  = MColor(values.at(0).getUInt());
                        colors[1]  = colors[3]  = MColor(values.at(1).getUInt());
                    } else {
                        colors[0]  = colors[2]  =
                            colors[1]  = colors[3]  = MColor(values.at(0).getUInt());
                    }
                }
                break;
            case PT_BorderTop:
            case PT_BorderRight:
            case PT_BorderBottom:
            case PT_BorderLeft:
                {
                    int index = declIter->first - PT_BorderTop;
                    for(unsigned int i = 0; i < values.size(); ++i)
                    {
                        if(values.at(i).type == CssValue::Color) {
                            colors[index]  = MColor(values.at(0).getUInt());

                        } else if(values.at(i).type == CssValue::Identifier)
                        {
                            ValueType vi = values.at(0).getIdentifier();
                            if(index == 0)      obj->styles.top    = vi;
                            else if(index == 1) obj->styles.right  = vi;
                            else if(index == 2) obj->styles.bottom = vi;
                            else obj->styles.left = vi;
                        } else
                            setRectUValue(obj->widths,index,values.at(i).getUInt());
                    }
                }
                break;
            case PT_BorderTopColor:
            case PT_BorderRightColor:
            case PT_BorderBottomColor:
            case PT_BorderLeftColor: 
                colors[declIter->first - PT_BorderTopColor]  = MColor(values.at(0).getUInt());
                break;
            case PT_BorderTopWidth:
            case PT_BorderRightWidth:
            case PT_BorderBottomWidth:
            case PT_BorderLeftWidth:
                setRectUValue(obj->widths, declIter->first - PT_BorderTopWidth, values.at(0).getUInt());
                break;
            case PT_BorderTopStyle:
            case PT_BorderRightStyle:
            case PT_BorderBottomStyle:
            case PT_BorderLeftStyle:
                setRectUValue(obj->styles, declIter->first - PT_BorderTopStyle, values.at(0).getIdentifier());
                break;
            case PT_BorderTopLeftRadius:
            case PT_BorderTopRightRadius:
            case PT_BorderBottomLeftRadius:
            case PT_BorderBottomRightRadius:
                obj->radiuses[declIter->first - PT_BorderTopLeftRadius] = values.at(0).getUInt();
                break;
            }
            ++declIter;
        }

        if(obj->widths.left  == obj->widths.right  &&
            obj->widths.right == obj->widths.top    && 
            obj->widths.top   == obj->widths.bottom &&
            colors[0] == colors[1] &&
            colors[1] == colors[2] && 
            colors[2] == colors[3]) obj->uniform = true;

        for(int i = 0; i < 4; ++i) {
            obj->brushes[i] = MD2DBrushHandle::create(colors[i]);
            if(!colors[i].isTransparent())
                obj->isTransparent = false;
        }
    }

    void RenderRuleData::init(DeclMap& declarations)
    {
        // Remark: If the declaration have values in wrong order, it might crash the program.
        // Maybe we should add some logic to avoid this flaw.
        DeclMap::iterator declIter    = declarations.begin();
        DeclMap::iterator declIterEnd = declarations.end();

        vector<BackgroundOpaqueType> bgOpaqueTypes;
        bool opaqueBorderImage = false;
        BorderType bt = BT_Simple;

        // --- Backgrounds ---
        while(declIter->first == PT_Background)
        {
            CssValueArray& values = declIter->second->values;
            if(values.at(0).type == CssValue::Identifier &&
                values.at(0).getIdentifier() == Value_None)
            {
                do {
                    ++declIter;
                    if(declIter == declIterEnd) goto END;
                } while(declIter->first <= PT_BackgroundAlignment);
            } else
            {
                BackgroundOpaqueType bgOpaqueType = NonOpaque;
                bool forceOpaque = false;
                BackgroundRenderObject* o = createBackgroundRO(values, &forceOpaque);
                if(forceOpaque || isBrushValueOpaque(values.at(0)))
                {
                    if(testValue(o, Value_Margin))
                        bgOpaqueType = OpaqueClipMargin;
                    else if(testValue(o, Value_Border))
                        bgOpaqueType = OpaqueClipBorder;
                    else
                        bgOpaqueType = OpaqueClipPadding;
                }

                backgroundROs.push_back(o);
                bgOpaqueTypes.push_back(bgOpaqueType);

                if(++declIter == declIterEnd) goto END;
            }

        }
        while(declIter->first < PT_BackgroundPosition)
        {
            batchSetBGROProps(backgroundROs, declIter->second->values.at(0).getIdentifier());
            if(++declIter == declIterEnd) goto END;
        }
        if(declIter->first == PT_BackgroundPosition)
        {
            vector<CssValue>& values = declIter->second->values;
            int propx = values.at(0).getInt();
            int propy = values.size() == 1 ? propx : values.at(1).getInt();

            vector<BackgroundRenderObject*>::iterator it    = backgroundROs.begin();
            vector<BackgroundRenderObject*>::iterator itEnd = backgroundROs.end();
            while(it != itEnd) {
                (*it)->x = propx;
                (*it)->y = propy;
                ++it;
            }
            if(++declIter == declIterEnd) goto END;
        }
        if(declIter->first == PT_BackgroundSize)
        {
            vector<CssValue>& values = declIter->second->values;
            int propw = values.at(0).getUInt();
            int proph = values.size() == 1 ? propw : values.at(1).getUInt();

            vector<BackgroundRenderObject*>::iterator it    = backgroundROs.begin();
            vector<BackgroundRenderObject*>::iterator itEnd = backgroundROs.end();
            while(it != itEnd) {
                (*it)->width  = propw;
                (*it)->height = proph;
                ++it;
            }
            if(++declIter == declIterEnd) goto END;
        }
        if(declIter->first == PT_BackgroundAlignment)
        {
            vector<CssValue>& values = declIter->second->values;
            batchSetBGROProps(backgroundROs,values.at(0).getIdentifier());
            if(values.size() == 2)
                batchSetBGROProps(backgroundROs,values.at(1).getIdentifier(),true);

            if(++declIter == declIterEnd) goto END;
        }

        // --- BorderImage --- 
        if(declIter->first == PT_BorderImage)
        {
            CssValueArray& values = declIter->second->values;
            if(values.at(0).type == CssValue::Uri)
            {
                createBorderImageRO(declIter->second->values, &opaqueBorderImage);
                if(!opaqueBorderImage)
                    opaqueBorderImage = isBrushValueOpaque(values.at(0));
            }
            if(++declIter == declIterEnd) goto END;
        }

        // --- Border ---
        if(declIter->first == PT_Border)
        {
            CssValue& v = declIter->second->values.at(0);
            if(v.type == CssValue::Identifier  && v.getUInt() == Value_None)
            { // User specifies no border. Skip everything related to border.
                do {
                    if(++declIter == declIterEnd) goto END;
                } while (declIter->first <= PT_BorderRadius);
            }
        }

        if(declIter->first <= PT_BorderRadius)
        {
            bt = testBorderObjectType(declIter,declIterEnd);

            DeclMap::iterator declIter2 = declIter;
            DeclMap::iterator tempDeclIter = declIter2;
            ++tempDeclIter;

            if(bt == BT_Simple)
            {
                while(tempDeclIter != declIterEnd && tempDeclIter->first <= PT_BorderStyles)
                { declIter2 = tempDeclIter; ++tempDeclIter; }
                borderRO = new SimpleBorderRenderObject();
                setSimpleBorderRO(declIter,declIter2);

            } else if(bt == BT_Radius)
            {
                while(tempDeclIter != declIterEnd && tempDeclIter->first <= PT_BorderRadius)
                { declIter2 = tempDeclIter; ++tempDeclIter; }
                RadiusBorderRenderObject* obj = new RadiusBorderRenderObject();
                obj->radius = declIter2->second->values.at(0).getUInt();
                borderRO = obj;
                setSimpleBorderRO(declIter,declIter2);

            } else
            {
                while(tempDeclIter != declIterEnd && tempDeclIter->first <= PT_BorderRadius)
                { declIter2 = tempDeclIter; ++tempDeclIter; }
                borderRO = new ComplexBorderRenderObject();
                setComplexBorderRO(declIter, declIter2);
            }

            if(declIter == declIterEnd) goto END;
        }

        // --- Margin & Padding --- 
        while(declIter->first <= PT_PaddingLeft)
        {
            vector<CssValue>& values = declIter->second->values;
            if(values.at(0).type != CssValue::Identifier ||
                values.at(0).getIdentifier() != Value_None)
            {
                switch(declIter->first)
                {
                case PT_Margin:
                    if(margin == 0)
                        margin = new MRectU();
                    setRectUValue(*margin,values);
                    break;
                case PT_MarginTop:
                case PT_MarginRight:
                case PT_MarginBottom:
                case PT_MarginLeft:
                    if(margin == 0)
                        margin = new MRectU();
                    setRectUValue(*margin, declIter->first - PT_MarginTop,
                        values.at(0).getUInt());
                    break;
                case PT_Padding:
                    if(padding == 0)
                        padding = new MRectU();
                    setRectUValue(*padding,values);
                    break;
                case PT_PaddingTop:
                case PT_PaddingRight:
                case PT_PaddingBottom:
                case PT_PaddingLeft:
                    if(padding == 0)
                        padding = new MRectU();
                    setRectUValue(*padding, declIter->first - PT_PaddingTop,
                        values.at(0).getUInt());
                    break;
                }
            }
            if(++declIter == declIterEnd) goto END;
        }

        // --- Geometry ---
        if(declIter->first <= PT_MaximumHeight)
        {
            geoRO = new GeometryRenderObject();
            do {
                CssValueArray& values = declIter->second->values;
                if(values.at(0).type != CssValue::Identifier ||
                    values.at(0).getIdentifier() != Value_None)
                {
                    int data = values.at(0).getInt();
                    switch(declIter->first)
                    {

                    case PT_PosX:         geoRO->x         = data; break;
                    case PT_PosY:         geoRO->y         = data; break;
                    case PT_Width:        geoRO->width     = data; break;
                    case PT_Height:       geoRO->height    = data; break;
                    case PT_MinimumWidth: geoRO->minWidth  = data; break;
                    case PT_MinimumHeight:geoRO->minHeight = data; break;
                    case PT_MaximumWidth: geoRO->maxWidth  = data; break;
                    case PT_MaximumHeight:geoRO->maxHeight = data; break;
                    }
                }

                if(++declIter == declIterEnd) goto END;
            } while(declIter->first <= PT_MaximumHeight);
        }

        // --- Cursor ---
        if(declIter->first == PT_Cursor)
        {
            cursor = new MCursor();
            const CssValue& value = declIter->second->values.at(0);
            switch(value.type)
            {
            case CssValue::Identifier:
                if(value.getIdentifier() < Value_Default || value.getIdentifier() > Value_Blank)
                {
                    delete cursor;
                    cursor = 0;
                } else {
                    cursor->setType(static_cast<MCursor::CursorType>
                        (value.getIdentifier() - Value_Default));
                }
                break;
            case CssValue::Uri:
                cursor->loadCursorFromFile(*value.data.vstring);
                break;
            case CssValue::Number:
                cursor->loadCursorFromRes(MAKEINTRESOURCEW(value.getInt()));
                break;
            default:
                delete cursor;
                cursor = 0;
            }
            if(++declIter == declIterEnd) goto END;
        }

        // --- Text ---
        if(declIter->first <= PT_TextShadow)
        {
            wstring fontFace = L"Arial";
            bool bold      = false;
            bool italic    = false;
            bool pixelSize = false;
            unsigned int size = 12;
            while(declIter != declIterEnd && declIter->first <= PT_FontWeight)
            {
                const CssValue& value = declIter->second->values.at(0);
                switch(declIter->first)
                {
                case PT_Font:
                    {
                        const CssValueArray& values = declIter->second->values;
                        for(int i = values.size() - 1; i >= 0; --i)
                        {
                            const CssValue& v = values.at(i);
                            if(v.type == CssValue::String)
                                fontFace = *v.data.vstring;
                            else if(v.type == CssValue::Identifier) {
                                if(v.getIdentifier() == Value_Bold)
                                    bold = true;
                                else if(v.getIdentifier() == Value_Italic || 
                                    v.getIdentifier() == Value_Oblique)
                                    italic = true;
                            } else {
                                size = v.getUInt();
                                pixelSize = v.type == CssValue::Length;
                            }
                        }

                    }
                    break;
                case PT_FontSize:
                    size = value.getUInt();
                    // If the value is 12px, the CssValue is Length.
                    // If the value is 16, the CssValue is Number.
                    // We don't support unit 'pt'.
                    pixelSize = (value.type == CssValue::Length);
                    break;
                case PT_FontStyle: italic = (value.getIdentifier() != Value_Normal); break;
                case PT_FontWeight:  bold = (value.getIdentifier() == Value_Bold);   break;
                }
                ++declIter;
            }

            MFont font(size,fontFace, bold, italic, pixelSize);
            textRO = new TextRenderObject(font);
            if(declIter == declIterEnd) goto END;

            while(declIter != declIterEnd /*&& declIter->first <= PT_TextShadow*/)
            {
                const CssValueArray& values = declIter->second->values;
                switch(declIter->first)
                {
                case PT_Color:
                    textRO->color = values.at(0).getColor();
                    textRO->textBrush = MD2DBrushHandle::create(textRO->color);
                    break;
                case PT_TextShadow:
                    if(values.size() < 2)
                        break;
                    textRO->shadowOffsetX = (char)values.at(0).getInt();
                    textRO->shadowOffsetY = (char)values.at(1).getInt();
                    for(size_t i = 2; i < values.size(); ++i)
                    {
                        if(values.at(i).type == CssValue::Color)
                        {
                            textRO->shadowColor = values.at(i).getColor();
                            textRO->shadowBrush = MD2DBrushHandle::create(textRO->shadowColor);
                        } else
                            textRO->shadowBlur = (char)values.at(i).getUInt();
                    }
                    break;
                case PT_TextOutline:
                    textRO->outlineWidth = (char)values.at(0).getUInt();
                    textRO->outlineColor = values.at(values.size() - 1).getColor();
                    textRO->outlineBrush = MD2DBrushHandle::create(textRO->outlineColor);
                    if(values.size() >= 3)
                        textRO->outlineBlur = (char)values.at(1).getUInt();
                    break;
                case PT_TextAlignment:
                    if(values.size()>1)
                    {
                        switch(values.at(1).getIdentifier()) {
                        case Value_Top:    textRO->values |= Value_Top;     break;
                        case Value_Center: textRO->values |= Value_VCenter; break;
                        case Value_Bottom: textRO->values |= Value_Bottom;  break;
                        }
                        switch(values.at(0).getIdentifier()) {
                        case Value_Left:   textRO->values |= Value_Left;    break;
                        case Value_Center: textRO->values |= Value_HCenter; break;
                        case Value_Right:  textRO->values |= Value_Right;   break;
                        }
                    } else {
                        switch(values.at(0).getIdentifier()) {
                        case Value_Center:
                            textRO->values |= (Value_HCenter | Value_VCenter); break;
                        case Value_Left:
                        case Value_Right:
                        case Value_Top:
                        case Value_Bottom:
                            textRO->values |= values.at(0).getIdentifier();
                        }
                    }
                    break;
                case PT_TextOverflow:
                case PT_TextDecoration:
                    textRO->values |= values.at(0).getIdentifier(); break;
                case PT_TextUnderlineStyle:
                    textRO->lineStyle = values.at(0).getIdentifier(); break;
                }
                ++declIter;
            }
        }

    END:// Check if this RenderRule is opaque.
        int  opaqueCount  = 0;
        bool opaqueBorder = (!hasMargin() && bt == BT_Simple);
        for(int i = bgOpaqueTypes.size() - 1; i >= 0 && opaqueCount == 0; --i)
        {
            switch(bgOpaqueTypes.at(i)) {
            case OpaqueClipMargin:  ++opaqueCount; break;
            case OpaqueClipBorder:  if(opaqueBorder) ++opaqueCount; break;
            case OpaqueClipPadding:
                if(opaqueBorder && !hasPadding())
                    ++opaqueCount;
                break;
            }
        }
        if(opaqueBorderImage) ++opaqueCount;
        opaqueBackground = opaqueCount > 0;
    }

    // ********** RenderRule Impl
    TextRenderer RenderRule::textRenderer = AutoDetermine;
    unsigned int RenderRule::maxGdiFontPtSize = 12;
    RenderRule::RenderRule(RenderRuleData* d):data(d)
        { if(data) ++data->refCount; }
    RenderRule::RenderRule(const RenderRule& d):data(d.data)
        { if(data) ++data->refCount; }
    bool RenderRule::opaqueBackground() const
        { return data == 0 ? false : data->opaqueBackground; }
    void RenderRule::draw(MD2DPaintContext& c,const MRect& wr, const MRect& cr,
        const wstring& t,unsigned int i)
        { if(data) data->draw(c,wr,cr,t,i); }
    void RenderRule::init()
        { M_ASSERT(data==0); data = new RenderRuleData(); }
    MCursor* RenderRule::getCursor()
        { return data ? data->cursor : 0; }
    MSize RenderRule::getStringSize(const std::wstring& s, int maxWidth)
        { return data ? data->getStringSize(s, maxWidth) : MSize(); }
    MRect RenderRule::getContentMargin()
        { return data ? data->getContentMargin() : MRect(); }
    RenderRule::~RenderRule()
    { 
        if(data && --data->refCount == 0)
            delete data;
    }
    const RenderRule& RenderRule::operator=(const RenderRule& rhs)
    {
        if(data == rhs.data) return rhs;
        if(data && --data->refCount == 0) { delete data; }
        if((data = rhs.data) != 0) ++data->refCount;
        return *this;
    }

    int RenderRule::getGeometry(Geometry geo)
    {
        if(data == 0 || data->geoRO == 0)
        {
            if(geo == RRG_X || geo == RRG_Y)
                return INT_MAX;
            else
                return -1;
        }

        switch(geo)
        {
            case RRG_X:         return data->geoRO->x;
            case RRG_Y:         return data->geoRO->y;
            case RRG_Width:     return data->geoRO->width;
            case RRG_Height:    return data->geoRO->height;
            case RRG_MinWidth:  return data->geoRO->minWidth;
            case RRG_MinHeight: return data->geoRO->minHeight;
            case RRG_MaxWidth:  return data->geoRO->maxWidth;
            case RRG_MaxHeight: return data->geoRO->maxHeight;
        }
        return -1;
    }

    // ********** RenderRuleQuerier Impl 
    RenderRuleQuerier::~RenderRuleQuerier()
    {
        if(mApp)
            mApp->getStyleSheet()->removeCache(this);

        for(size_t i = 0; i < children.size(); ++i)
            delete children.at(i);
    }

    // ********** RenderRuleCacheKey Impl 
    bool RenderRuleCacheKey::operator<(const RenderRuleCacheKey& rhs) const
    {
        if(this == &rhs)
            return false;

        int size1 = styleRules.size();
        int size2 = rhs.styleRules.size();
        if(size1 > size2) {
            return false;
        } else if(size1 < size2) {
            return true;
        } else {
            for(int i = 0; i < size1; ++i)
            {
                StyleRule* sr1 = styleRules.at(i);
                StyleRule* sr2 = rhs.styleRules.at(i);
                if(sr1 > sr2)
                    return false;
                else if(sr1 < sr2)
                    return true;
            }
        }
        return false;
    }
} // namespace MetalBone