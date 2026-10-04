// Scalable, code-drawn icons shared by gameplay and the layout editor.
// The pinned RmlUi accepts lengths, NOT percentages, for border-radius.
// Large dp radii are clamped to each shape's box, producing round ends at any size.
std::string extra_icon_part(const char* style) {
    return "<span style='position:absolute;display:block;pointer-events:none;" +
        std::string(style) + "'></span>";
}
std::string extra_button_content(size_t index) {
    if (index != touch::Jump && index != touch::DarkLink)
        return "<span>" + std::string(touch::Labels[index]) + "</span>";
    std::string icon = "<span style='position:relative;display:block;width:86%;height:86%;pointer-events:none;'>";
    if (index == touch::Jump) {
        const char* parts[] = {
            // Short rising trails make the airborne pose distinct from running.
            "left:14%;top:70%;width:5%;height:17%;border-radius:100dp;background-color:#879b9b;transform:rotate(24deg);",
            "left:5%;top:63%;width:4%;height:12%;border-radius:100dp;background-color:#657e80;transform:rotate(24deg);",
            // Rear arm and bent rear leg, shaded behind the near limbs.
            "left:23%;top:27%;width:27%;height:11%;border-radius:100dp;background-color:#b6c1bb;transform:rotate(36deg);",
            "left:13%;top:15%;width:11%;height:24%;border-radius:100dp;background-color:#b6c1bb;transform:rotate(-18deg);",
            "left:13%;top:10%;width:12%;height:13%;border-radius:100dp;background-color:#d8ded0;",
            "left:28%;top:58%;width:28%;height:13%;border-radius:100dp;background-color:#b6c1bb;transform:rotate(-36deg);",
            "left:20%;top:69%;width:22%;height:12%;border-radius:100dp;background-color:#b6c1bb;transform:rotate(26deg);",
            "left:14%;top:64%;width:12%;height:21%;border-radius:4dp 4dp 2dp 6dp;background-color:#d8ded0;transform:rotate(-36deg);",
            // Torso/neck and round head. Slight forward lean, with both arms raised.
            "left:43%;top:22%;width:11%;height:14%;border-radius:100dp;background-color:#e2e5d8;transform:rotate(12deg);",
            "left:37%;top:30%;width:21%;height:32%;border-radius:7dp 7dp 4dp 4dp;background-color:#f8f4e8;transform:rotate(12deg);",
            "left:40%;top:4%;width:23%;height:23%;border-radius:100dp;background-color:#f8f4e8;",
            "left:44%;top:7%;width:8%;height:5%;border-radius:100dp;background-color:#ffffff;transform:rotate(-28deg);",
            // Raised near arm, elbow and fist.
            "left:52%;top:27%;width:25%;height:12%;border-radius:100dp;background-color:#f8f4e8;transform:rotate(-36deg);",
            "left:68%;top:13%;width:12%;height:23%;border-radius:100dp;background-color:#f8f4e8;transform:rotate(20deg);",
            "left:73%;top:7%;width:13%;height:14%;border-radius:100dp;background-color:#ffffff;transform:rotate(20deg);",
            // Near knee lifted forward, lower leg and defined boot/foot.
            "left:47%;top:56%;width:28%;height:14%;border-radius:100dp;background-color:#f8f4e8;transform:rotate(27deg);",
            "left:68%;top:64%;width:12%;height:24%;border-radius:100dp;background-color:#f8f4e8;transform:rotate(-7deg);",
            "left:69%;top:82%;width:20%;height:11%;border-radius:3dp 7dp 3dp 3dp;background-color:#ffffff;transform:rotate(8deg);",
        };
        for (const auto* part : parts) icon += extra_icon_part(part);
    } else {
        const char* parts[] = {
            // Swept cap: a folded tip and curved crown instead of a square tower.
            "left:51%;top:5%;width:37%;height:25%;border-radius:100dp 2dp 100dp 10dp;background-color:#1e292f;transform:rotate(-18deg);",
            "left:70%;top:11%;width:22%;height:12%;border-radius:100dp 1dp 100dp 2dp;background-color:#10181e;transform:rotate(22deg);",
            "left:26%;top:13%;width:49%;height:32%;border-radius:100dp 100dp 5dp 5dp;background-color:#161f26;transform:rotate(-9deg);",
            "left:38%;top:17%;width:28%;height:3%;border-radius:100dp;background-color:#344149;transform:rotate(-15deg);",
            // Hylian ears with a small inner fold, visible against a dark scene.
            "left:7%;top:40%;width:28%;height:20%;border-radius:1dp 100dp 3dp 100dp;background-color:#354047;transform:rotate(16deg);",
            "left:65%;top:40%;width:28%;height:20%;border-radius:100dp 1dp 100dp 3dp;background-color:#354047;transform:rotate(-16deg);",
            "left:12%;top:44%;width:20%;height:11%;border-radius:1dp 100dp 2dp 100dp;background-color:#0b1117;transform:rotate(16deg);",
            "left:68%;top:44%;width:20%;height:11%;border-radius:100dp 1dp 100dp 2dp;background-color:#0b1117;transform:rotate(-16deg);",
            // Tapered jaw and subtly outlined face; the interior stays nearly black.
            "left:34%;top:64%;width:32%;height:27%;border-radius:4dp 4dp 8dp 4dp;background-color:#303b42;transform:rotate(45deg);",
            "left:27%;top:30%;width:47%;height:51%;border-radius:100dp 100dp 20dp 20dp;background-color:#354047;",
            "left:35%;top:63%;width:30%;height:26%;border-radius:4dp 4dp 7dp 4dp;background-color:#080d12;transform:rotate(45deg);",
            "left:29%;top:32%;width:43%;height:48%;border-radius:100dp 100dp 20dp 20dp;background-color:#0a1016;",
            // Layered fringe and pointed side locks frame the eyes.
            "left:23%;top:27%;width:54%;height:19%;border-radius:100dp 100dp 4dp 4dp;background-color:#111a22;transform:rotate(-5deg);",
            "left:29%;top:30%;width:13%;height:28%;border-radius:10dp 2dp 1dp 10dp;background-color:#1e2b34;transform:rotate(31deg);",
            "left:43%;top:29%;width:13%;height:24%;border-radius:8dp 2dp 1dp 10dp;background-color:#17232c;transform:rotate(29deg);",
            "left:57%;top:29%;width:13%;height:23%;border-radius:10dp 2dp 1dp 10dp;background-color:#202d36;transform:rotate(-28deg);",
            "left:24%;top:41%;width:10%;height:31%;border-radius:9dp 2dp 1dp 9dp;background-color:#17232c;transform:rotate(5deg);",
            "left:67%;top:40%;width:10%;height:31%;border-radius:2dp 9dp 9dp 1dp;background-color:#17232c;transform:rotate(-5deg);",
            // Red eye sockets, slanted luminous eyes and tiny pale-red cores.
            "left:33%;top:53%;width:16%;height:11%;border-radius:100dp;background-color:#57121d;transform:rotate(13deg);",
            "left:53%;top:53%;width:16%;height:11%;border-radius:100dp;background-color:#57121d;transform:rotate(-13deg);",
            "left:34%;top:55%;width:14%;height:5%;border-radius:1dp 2dp 3dp 2dp;background-color:#ff243f;transform:rotate(13deg);",
            "left:54%;top:55%;width:14%;height:5%;border-radius:2dp 1dp 2dp 3dp;background-color:#ff243f;transform:rotate(-13deg);",
            "left:40%;top:55%;width:4%;height:4%;border-radius:100dp;background-color:#ffc2c6;",
            "left:58%;top:55%;width:4%;height:4%;border-radius:100dp;background-color:#ffc2c6;",
            // Restrained nose, mouth and collar details remain readable at 56dp.
            "left:49%;top:62%;width:3%;height:10%;border-radius:1dp;background-color:#29323b;transform:rotate(-8deg);",
            "left:43%;top:77%;width:15%;height:2%;border-radius:100dp;background-color:#41444b;",
            "left:39%;top:89%;width:24%;height:5%;border-radius:2dp 2dp 6dp 6dp;background-color:#192630;",
        };
        for (const auto* part : parts) icon += extra_icon_part(part);
    }
    return icon + "</span>";
}
