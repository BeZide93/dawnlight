// File info is shared by file select, copy destinations and the native save
// menu. Read the actual slot, never the most recently loaded s_records.
constexpr u64 kSaveMetaTag=MULTI_CHAR('dl_meta');
constexpr std::array<u64,20> kSaveHeartTags{
    MULTI_CHAR('hear_20'),MULTI_CHAR('hear_21'),MULTI_CHAR('hear_22'),MULTI_CHAR('hear_23'),MULTI_CHAR('hear_24'),
    MULTI_CHAR('hear_25'),MULTI_CHAR('hear_26'),MULTI_CHAR('hear_27'),MULTI_CHAR('hear_28'),MULTI_CHAR('hear_29'),
    MULTI_CHAR('hear_30'),MULTI_CHAR('hear_31'),MULTI_CHAR('hear_32'),MULTI_CHAR('hear_33'),MULTI_CHAR('hear_34'),
    MULTI_CHAR('hear_35'),MULTI_CHAR('hear_36'),MULTI_CHAR('hear_37'),MULTI_CHAR('hear_38'),MULTI_CHAR('hear_39')};

bool save_heart_bounds(J2DPane* heart,J2DPane* parent,JGeometry::TBox2<f32>& area,bool& first) {
    const auto& bounds=heart->getBounds();
    for(unsigned corner=0;corner<4;++corner) {
        float x=corner&1?bounds.f.x:bounds.i.x,y=corner&2?bounds.f.y:bounds.i.y;
        auto* pane=heart;
        // Transform into the data panel's coordinates, including intermediate
        // pane offsets/scales. The label then inherits native slot fades/motion.
        for(unsigned depth=0;pane && pane!=parent && depth<32;++depth) {
            const auto& m=pane->mPositionMtx;
            const float nextX=m[0][0]*x+m[0][1]*y+m[0][3];
            y=m[1][0]*x+m[1][1]*y+m[1][3];x=nextX;
            pane=pane->getParentPane();
        }
        if(pane!=parent || !std::isfinite(x) || !std::isfinite(y))return false;
        if(first){area=JGeometry::TBox2<f32>(x,y,x,y);first=false;}
        else {area.i.x=std::min(area.i.x,x);area.i.y=std::min(area.i.y,y);
              area.f.x=std::max(area.f.x,x);area.f.y=std::max(area.f.y,y);}
    }
    return true;
}

void after_survivors_save_slot(ModContext*,void* args,void* result,void*) {
    auto* info=mods::arg<dFile_info_c*>(args,0);
    if(!info || !info->mFileInfo.Scr)return;
    auto* screen=info->mFileInfo.Scr;
    auto* label=static_cast<J2DTextBox*>(screen->search(kSaveMetaTag));
    if(label)label->hide();
    const auto slot=mods::arg<u8>(args,3);
    const int status=result?*static_cast<int*>(result):-1;
    if(!s_active || slot>=3 || (status!=0 && status!=2) || !svc_save || !svc_save->peek_blob)return;

    std::array<uint8_t,Records::byte_count> bytes{};size_t size=bytes.size();
    Records records;
    const auto read=svc_save->peek_blob(mod_ctx,slot,kBlob,bytes.data(),&size);
    const bool known=read==MOD_UNAVAILABLE ||
        (read==MOD_OK && size<=bytes.size() && records.decode(std::span(bytes.data(),size)));
    char text[32];
    if(known)std::snprintf(text,sizeof(text),"Meta Level %u",records.level());
    else std::snprintf(text,sizeof(text),"Meta Level ?");

    auto* parent=info->mFileInfo.field_0x10;
    auto* style=static_cast<J2DTextBox*>(screen->search(MULTI_CHAR('w_ptim01')));
    if(!parent || !style || !info->mFileInfo.mFont)return;
    std::array<J2DPane*,20> hearts{};
    JGeometry::TBox2<f32> area;bool first=true;
    for(unsigned i=0;i<hearts.size();++i) {
        hearts[i]=screen->search(kSaveHeartTags[i]);
        if(!hearts[i] || !save_heart_bounds(hearts[i],parent,area,first))return;
    }
    if(area.getWidth()<=0 || area.getHeight()<=0)return;
    if(!label) {
        label=JKR_NEW J2DTextBox(kSaveMetaTag,area,nullptr,"",sizeof(text),HBIND_LEFT,VBIND_CENTER);
        if(!label)return;
        if(!parent->appendChild(label)){JKR_DELETE(label);return;}
        // The native pane tree owns and deletes this native-class textbox.
        label->setFont(info->mFileInfo.mFont);
        label->setInfluencedAlpha(true,false);
    }
    // area is in the parent's local coordinates. Native place() accepts an
    // offset from its top-left and adds parent->getBounds().i itself.
    const auto& origin=parent->getBounds().i;
    label->place(JGeometry::TBox2<f32>(area.i.x-origin.x,area.i.y-origin.y,
        area.f.x-origin.x,area.f.y-origin.y));
    J2DTextBox::TFontSize font;style->getFontSize(font);
    const float scale=std::min({1.f,area.getHeight()/std::max(1.f,font.mSizeY),
        area.getWidth()/std::max(1.f,font.mSizeX*float(std::strlen(text)))});
    label->setFontSize(font.mSizeX*scale,font.mSizeY*scale);
    label->setFontColor(style->getCharColor(),style->getGradColor());
    // Despite its variadic signature, the native setter copies literal text.
    label->setString(sizeof(text),text);
    if(!label->getStringPtr())return;
    for(auto* heart:hearts)heart->hide();
    label->show();
}
