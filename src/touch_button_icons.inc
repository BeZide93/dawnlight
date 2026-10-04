// Code-drawn icons share the same scalable shapes in gameplay and the editor.
// No texture assets or platform-dependent emoji/fonts are needed.
std::string extra_icon_part(const char* style) {
    return "<span style='position:absolute;display:block;pointer-events:none;" +
        std::string(style) + "'></span>";
}
std::string extra_button_content(size_t index) {
    if (index != touch::Jump && index != touch::DarkLink)
        return "<span>" + std::string(touch::Labels[index]) + "</span>";
    std::string icon = "<span style='position:relative;width:82%;height:82%;pointer-events:none;'>";
    if (index == touch::Jump) {
        const char* parts[] = {
            "left:42%;top:5%;width:20%;height:20%;border-radius:50%;background-color:#f8f4e8;",
            "left:43%;top:29%;width:16%;height:32%;border-radius:3dp;background-color:#f8f4e8;transform:rotate(12deg);",
            "left:21%;top:31%;width:31%;height:11%;border-radius:3dp;background-color:#f8f4e8;transform:rotate(-35deg);",
            "left:56%;top:22%;width:31%;height:11%;border-radius:3dp;background-color:#f8f4e8;transform:rotate(-55deg);",
            "left:28%;top:61%;width:28%;height:12%;border-radius:3dp;background-color:#f8f4e8;transform:rotate(-45deg);",
            "left:10%;top:69%;width:28%;height:11%;border-radius:3dp;background-color:#f8f4e8;transform:rotate(20deg);",
            "left:51%;top:59%;width:28%;height:12%;border-radius:3dp;background-color:#f8f4e8;transform:rotate(32deg);",
            "left:67%;top:72%;width:11%;height:22%;border-radius:3dp;background-color:#f8f4e8;transform:rotate(-15deg);",
        };
        for (const auto* part : parts) icon += extra_icon_part(part);
    } else {
        const char* parts[] = {
            // Pointed cap, ears, face, sideburns and red eyes form a Link silhouette.
            "left:43%;top:3%;width:34%;height:38%;border-radius:0dp 2dp 18dp 2dp;background-color:#101418;transform:rotate(-28deg);",
            "left:13%;top:40%;width:23%;height:25%;border-radius:0dp 10dp 2dp 10dp;background-color:#07090b;transform:rotate(-25deg);",
            "left:66%;top:40%;width:23%;height:25%;border-radius:10dp 0dp 10dp 2dp;background-color:#07090b;transform:rotate(25deg);",
            "left:26%;top:29%;width:49%;height:60%;border-radius:42%;background-color:#07090b;",
            "left:25%;top:23%;width:52%;height:29%;border-radius:45% 45% 10% 10%;background-color:#101418;transform:rotate(-8deg);",
            "left:24%;top:42%;width:11%;height:35%;border-radius:0dp 0dp 0dp 8dp;background-color:#101418;",
            "left:67%;top:42%;width:11%;height:35%;border-radius:0dp 0dp 8dp 0dp;background-color:#101418;",
            "left:35%;top:55%;width:13%;height:7%;border-radius:2dp;background-color:#ff2028;transform:rotate(12deg);",
            "left:54%;top:55%;width:13%;height:7%;border-radius:2dp;background-color:#ff2028;transform:rotate(-12deg);",
        };
        for (const auto* part : parts) icon += extra_icon_part(part);
    }
    return icon + "</span>";
}
