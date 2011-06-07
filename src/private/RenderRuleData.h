#pragma once
#include "CSSParser.h"
#include "MD2DUtils.h"
#include "MUtils.h"

#include <map>
#include <d2d1.h>

using namespace std;
using namespace std::tr1;

namespace MetalBone
{
    class MRect;
    namespace CSS
    {
        enum   PropertyType;
        struct Declaration;
        struct GeometryRenderObject;
        struct BackgroundRenderObject;
        struct BorderImageRenderObject;
        struct BorderRenderObject;
        struct SimpleBorderRenderObject;
        struct RadiusBorderRenderObject;
        struct ComplexBorderRenderObject;
        struct TextRenderObject;

        /*
         * It is possible to manually create a RenderRuleData object, then use
         * it to do the drawing.
         **/
        class RenderRuleData
        {
            public:
                inline RenderRuleData();
                ~RenderRuleData();

                void draw(MD2DPaintContext&, const MRect& widgetRectInRT,
                    const MRect& clipRectInRT, const wstring& text, unsigned int frameIndex);

                // Return true if we changed the widget's size.
                bool  setGeometry(MWidget*);
                // Calc the string size according to the css text format.
                MSize getStringSize(const wstring&, int maxWidth);
                MRect getContentMargin();

                inline bool         hasMargin()          const;
                inline bool         hasPadding()         const;
                inline bool         isBGSingleLoop()     const;
                inline unsigned int getTotalFrameCount() const;

            public:
                vector<BackgroundRenderObject*> backgroundROs;
                BorderImageRenderObject*        borderImageRO;
                BorderRenderObject*             borderRO;
                GeometryRenderObject*           geoRO;
                TextRenderObject*               textRO;
                MCursor*                        cursor;
                MRectU*                         margin;
                MRectU*                         padding;

                int    refCount;
                int    totalFrameCount;
                bool   opaqueBackground;

            private:
                void drawBackgrounds(const MRect& widgetRect, const MRect& clipRect, ID2D1Geometry*, unsigned int frameIndex);
                void drawGdiText    (const D2D1_RECT_F& borderRect, const MRect& clipRect, const wstring& text);
                void drawD2DText    (const D2D1_RECT_F& borderRect, const wstring& text);

                // Initialization
                void init(multimap<PropertyType, Declaration*>&);
                BackgroundRenderObject* createBackgroundRO(CssValueArray&,bool* forceOpaque);
                void createBorderImageRO(CssValueArray&,bool* forceOpaque);
                void setSimpleBorderRO  (multimap<PropertyType, Declaration*>::iterator&, multimap<PropertyType, Declaration*>::iterator);
                void setComplexBorderRO (multimap<PropertyType, Declaration*>::iterator&, multimap<PropertyType, Declaration*>::iterator);

                static ID2D1RenderTarget* workingRT;

                friend class MSSSPrivate;
        };

        struct GeometryRenderObject
        {
            inline GeometryRenderObject();

            int x       , y;
            int width   , height;
            int minWidth, minHeight;
            int maxWidth, maxHeight;
        };

        struct BackgroundRenderObject
        {
            inline BackgroundRenderObject();

            // We remember the brush's pointer, so that if the brush is recreated,
            // we can recheck the background property again. But if a brush is recreated
            // at the same position, we won't know.
            MD2DBrushHandle brush;
            ID2D1Brush*    checkedBrush;
            unsigned int   x, y, width, height;
            unsigned int   values;
            unsigned short frameCount; // Do we need more frames?
            bool           infiniteLoop;
        };

        struct BorderImageRenderObject
        {
            inline BorderImageRenderObject();
            MD2DBrushHandle brush;
            MRect*         imageRect;
            unsigned int   values;
        };

        struct BorderRenderObject
        {
            enum BROType { SimpleBorder, RadiusBorder, ComplexBorder };
            BROType type;
            virtual ~BorderRenderObject() {}
            virtual void getBorderWidth(MRect&) const = 0;
            virtual bool isVisible()            const = 0;
        };

        struct SimpleBorderRenderObject : public BorderRenderObject
        {
            inline SimpleBorderRenderObject();

            MD2DBrushHandle brush;
            unsigned int   width;
            ValueType      style;
            bool           isColorTransparent;

            inline void getBorderWidth(MRect&) const;
            inline bool isVisible()            const;
            inline int  getWidth()             const;
        };
        struct RadiusBorderRenderObject : public SimpleBorderRenderObject
        {
            inline RadiusBorderRenderObject();
            int radius;
        };
        struct ComplexBorderRenderObject : public BorderRenderObject
        {
            ComplexBorderRenderObject();

            MD2DBrushHandle brushes[4];  // T, R, B, L
            MRectU         styles;
            MRectU         widths;
            unsigned int   radiuses[4]; // TL, TR, BL, BR
            bool           uniform;
            bool           isTransparent;

            void getBorderWidth(MRect&) const;
            bool isVisible()            const;

            ID2D1Geometry* createGeometry(const D2D1_RECT_F&);
            void draw(ID2D1RenderTarget*,ID2D1Geometry*,const D2D1_RECT_F&);
        };

        // TextRenderObject is intended to use GDI to render text,
        // so that we can take the benefit of GDI++ (not GDI+) to enhance
        // the text output quality.
        // Note: we can however draw the text with D2D, the text output looks
        // ok in my laptop, but ugly on my old 17" LCD screen. Anyway, I don't
        // know to to achieve blur effect with D2D yet.
        struct TextRenderObject
        {
            inline TextRenderObject(const MFont& f);

            MFont  font;
            MColor color;
            MColor outlineColor;
            MColor shadowColor;

            MD2DBrushHandle outlineBrush;
            MD2DBrushHandle shadowBrush;
            MD2DBrushHandle textBrush;

            unsigned int values;  // AlignmentXY, Decoration, OverFlow
            ValueType    lineStyle;

            short shadowOffsetX;
            short shadowOffsetY;
            char  shadowBlur;
            char  outlineWidth;
            char  outlineBlur;

            unsigned int       getGDITextFormat();
            IDWriteTextFormat* createDWTextFormat();
        };


        inline RenderRuleData::RenderRuleData():
            borderImageRO(0), borderRO(0), geoRO(0),
            textRO(0), cursor(0), margin(0), padding(0),
            refCount(1), totalFrameCount(1), opaqueBackground(false){}
        inline unsigned int RenderRuleData::getTotalFrameCount() const { return totalFrameCount; }
        inline bool RenderRuleData::hasMargin()                  const { return margin != 0; }
        inline bool RenderRuleData::hasPadding()                 const { return padding != 0; }
        inline bool RenderRuleData::isBGSingleLoop() const
        {
            for(unsigned int i = 0; i < backgroundROs.size(); ++i)
            {
                if(backgroundROs.at(i)->infiniteLoop)
                    return false;
            }
            return true;
        }
        inline BorderImageRenderObject::BorderImageRenderObject():
            imageRect(0),values(Value_Unknown){}
        inline GeometryRenderObject::GeometryRenderObject():
            x(INT_MAX),y(INT_MAX), 
            width(-1), height(-1), minWidth(-1),
            minHeight(-1), maxWidth(-1), maxHeight(-1){}
        inline TextRenderObject::TextRenderObject(const MFont& f):
            font(f), outlineColor(0), shadowColor(0),
            values(Value_Unknown), lineStyle(Value_Solid), 
            shadowOffsetX(0), shadowOffsetY(0), shadowBlur(0),
            outlineWidth(0), outlineBlur(0){}
        inline BackgroundRenderObject::BackgroundRenderObject():
            checkedBrush(0), x(0), y(0), width(0), height(0),
            values(Value_Top | Value_Left | Value_Margin),
            frameCount(1), infiniteLoop(true){}
        inline SimpleBorderRenderObject::SimpleBorderRenderObject():
            width(0),style(Value_Solid) { type = SimpleBorder; }
        inline RadiusBorderRenderObject::RadiusBorderRenderObject():
            radius(0){ type = RadiusBorder; }
        inline void SimpleBorderRenderObject::getBorderWidth(MRect& rect) const
            { rect.left = rect.right = rect.bottom = rect.top = width; }
        inline int  SimpleBorderRenderObject::getWidth() const { return width; }
        inline bool SimpleBorderRenderObject::isVisible() const
            { return !(width == 0 || isColorTransparent); }
    }
}