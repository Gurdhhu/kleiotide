#pragma once
#include <QColor>
#include <QString>
#include <array>
#include <algorithm>
#include <cmath>
#include <string_view>
class QJsonObject;
namespace kleiotide {
enum class ResiduePalette { Classic,HighContrast,OkabeIto,TolBright,Chromas,Custom };
// Number of built-in presets. Custom follows them in the palette selectors.
inline constexpr int residue_palette_count=5;
inline constexpr std::string_view custom_palette_symbols="ACGTURYSWKMBDHVN?";
using CustomResidueColors=std::array<QColor,custom_palette_symbols.size()>;
inline CustomResidueColors default_residue_colors(){
    CustomResidueColors colors;
    colors.fill(QColor(110,70,130));
    colors[0]=QColor(193,170,0);colors[1]=Qt::blue;colors[2]=Qt::black;colors[3]=colors[4]=Qt::red;
    return colors;
}
QJsonObject encode_residue_colors(const CustomResidueColors&);
CustomResidueColors decode_residue_colors(const QJsonObject&);
enum class ResidueDisplay { Letters,Monochrome,Backgrounds };
// Keep the legacy "classic" session ID when its visible name changes to Kleio.
inline QString palette_id(ResiduePalette p){const char* ids[]={"classic","high-contrast","okabe-ito","tol-bright","chromas","custom"};const auto i=static_cast<int>(p);return QString::fromLatin1(ids[i>=0&&i<=residue_palette_count?i:0]);}
inline QString palette_label(ResiduePalette p){const char* labels[]={"Kleio","High contrast","Okabe–Ito (color-blind-friendly)","Paul Tol bright (color-blind-friendly)","Chromas-style","Custom"};const auto i=static_cast<int>(p);return QString::fromUtf8(labels[i>=0&&i<=residue_palette_count?i:0]);}
inline ResiduePalette palette_from_id(const QString& id){for(int i=0;i<=residue_palette_count;++i)if(palette_id(static_cast<ResiduePalette>(i))==id)return static_cast<ResiduePalette>(i);return ResiduePalette::Classic;}
inline QString display_id(ResidueDisplay style){return style==ResidueDisplay::Monochrome?"monochrome":style==ResidueDisplay::Backgrounds?"backgrounds":"letters";}
inline ResidueDisplay display_from_id(const QString& id){return id=="monochrome"?ResidueDisplay::Monochrome:id=="backgrounds"?ResidueDisplay::Backgrounds:ResidueDisplay::Letters;}
// A, C, G, T. Okabe/Ito and Tol source URLs and text adaptations are in DISPLAY_COLORS.md.
inline std::array<QColor,4> palette_colors(ResiduePalette p,const CustomResidueColors* custom=nullptr){switch(p){
    case ResiduePalette::Custom:if(custom)return {(*custom)[0],(*custom)[1],(*custom)[2],(*custom)[3]};[[fallthrough]];
    case ResiduePalette::Classic:return {QColor(193,170,0),QColor(0,0,255),QColor(0,0,0),QColor(255,0,0)};
    case ResiduePalette::HighContrast:return {QColor("#8B6500"),QColor("#0050A4"),QColor("#202020"),QColor("#A00070")};
    case ResiduePalette::OkabeIto:return {QColor("#E69F00"),QColor("#0072B2"),QColor("#009E73"),QColor("#CC79A7")};
    case ResiduePalette::TolBright:return {QColor("#CCBB44"),QColor("#4477AA"),QColor("#228833"),QColor("#AA3377")};
    case ResiduePalette::Chromas:return {QColor("#008040"),QColor("#0000FF"),QColor("#000000"),QColor("#FF0000")};
    default:return {QColor(193,170,0),QColor(0,0,255),QColor(0,0,0),QColor(255,0,0)};
}}
inline double relative_luminance(const QColor& c){const auto linear=[](double v){return v<=0.04045?v/12.92:std::pow((v+0.055)/1.055,2.4);};return .2126*linear(c.redF())+.7152*linear(c.greenF())+.0722*linear(c.blueF());}
inline double color_contrast(const QColor& a,const QColor& b){const auto x=relative_luminance(a),y=relative_luminance(b);return (std::max(x,y)+.05)/(std::min(x,y)+.05);}
inline QColor on_white(QColor c){while(color_contrast(c,Qt::white)<4.5)c=QColor(static_cast<int>(c.red()*.95),static_cast<int>(c.green()*.95),static_cast<int>(c.blue()*.95));return c;}
inline QColor black_or_white(const QColor& background){return color_contrast(Qt::black,background)>=color_contrast(Qt::white,background)?QColor(Qt::black):QColor(Qt::white);}
inline int base_index(char c){if(c>='a'&&c<='z')c=static_cast<char>(c-'a'+'A');switch(c){case 'A':return 0;case 'C':return 1;case 'G':return 2;case 'T':case 'U':return 3;default:return -1;}}
inline bool display_gap(char c){return c=='-'||c=='~'||c=='.';}
inline QColor custom_residue_color(char symbol,const CustomResidueColors* custom){
    if(symbol>='a'&&symbol<='z')symbol=static_cast<char>(symbol-'a'+'A');
    const auto i=custom_palette_symbols.find(symbol);
    if(i==std::string_view::npos)return {};
    return custom?(*custom)[i]:default_residue_colors()[i];
}
inline QColor residue_ink(char c,ResiduePalette p,const CustomResidueColors* custom=nullptr){
    if(p==ResiduePalette::Custom){const auto color=custom_residue_color(c,custom);if(color.isValid())return color;}
    const auto index=base_index(c);if(index>=0){const auto color=palette_colors(p,custom)[static_cast<std::size_t>(index)];return p==ResiduePalette::Classic||p==ResiduePalette::Chromas?color:on_white(color);}if(display_gap(c))return p==ResiduePalette::Classic?QColor(192,192,192):QColor(102,102,102);return p==ResiduePalette::Classic?QColor(110,70,130):QColor(80,80,80);
}
struct ResiduePaint {QColor foreground,background;};
inline ResiduePaint residue_paint(char c,ResiduePalette palette,ResidueDisplay style,bool mask=false,const CustomResidueColors* custom=nullptr){
    if(mask)return {QColor(65,70,75),Qt::transparent};
    if(style==ResidueDisplay::Monochrome)return {display_gap(c)?QColor(102,102,102):QColor(32,32,32),Qt::transparent};
    if(style==ResidueDisplay::Letters)return {residue_ink(c,palette,custom),Qt::transparent};
    if(palette==ResiduePalette::Custom){
        const auto background=custom_residue_color(c,custom);
        if(background.isValid())return {black_or_white(background),background};
    }
    const auto index=base_index(c);const auto background=index>=0?palette_colors(palette,custom)[static_cast<std::size_t>(index)]:display_gap(c)?QColor(240,240,240):palette==ResiduePalette::Classic?QColor(110,70,130):QColor(216,216,216);
    return {black_or_white(background),background};
}
inline QColor opposite_color(const QColor& color){
    const auto hue=color.hslHueF();
    if(hue<0.)return QColor(255-color.red(),255-color.green(),255-color.blue());
    // Pair the red-to-green arc with its opposite arc. Yellow and blue
    // then correspond at the midpoints, while lightness is reversed.
    constexpr double green=1./3.;
    const double opposite=hue<green?green+2.*hue:(hue-green)/2.;
    const auto rgb=QColor::fromHslF(opposite,color.hslSaturationF(),1.-color.lightnessF());
    return QColor(rgb.red(),rgb.green(),rgb.blue());
}
}
