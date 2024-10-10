/*
 * Copyright (c) 2024 Huawei Device Co., Ltd.
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef OHOS_MEDIA_ALBUM_PLUGIN_DATA_H
#define OHOS_MEDIA_ALBUM_PLUGIN_DATA_H

#include <string>
#include <vector>

namespace OHOS::Media {
namespace AlbumPlugin {
struct AlbumPluginRowData {
    std::string lpath;
    std::string album_name;
    std::string album_name_en;
    std::string bundle_name;
    std::string cloud_id;
    std::string dual_album_name;
    int priority = 1;
};
const int CHAR_LOWCASE_H = 104;
const int CHAR_UPPERCASE_H = 72;
const int CHAR_LOWCASE_U = 117;
const int CHAR_LOWCASE_A = 97;
const int CHAR_LOWCASE_W = 119;
const int CHAR_LOWCASE_E = 101;
const int CHAR_LOWCASE_I = 105;
const std::string BRAND_NAME = std::string() + static_cast<char>(CHAR_LOWCASE_H) + static_cast<char>(CHAR_LOWCASE_U) +
    static_cast<char>(CHAR_LOWCASE_A) + static_cast<char>(CHAR_LOWCASE_W) +
    static_cast<char>(CHAR_LOWCASE_E) + static_cast<char>(CHAR_LOWCASE_I);
const std::string BRAND_NAME_UPPER_FIRST = std::string() + static_cast<char>(CHAR_UPPERCASE_H) +
    static_cast<char>(CHAR_LOWCASE_U) + static_cast<char>(CHAR_LOWCASE_A) +
    static_cast<char>(CHAR_LOWCASE_W) + static_cast<char>(CHAR_LOWCASE_E) +
    static_cast<char>(CHAR_LOWCASE_I);
const std::string LPATH_SCREEN_SHOTS = "/Pictures/Screenshots";
const std::string LPATH_SCREEN_RECORDS = "/Pictures/Screenrecords";
const std::string BUNDLE_NAME_SCREEN_RECORDS = "com." + BRAND_NAME + ".hmos.screenrecorder";
const std::string ALBUM_NAME_SCREEN_RECORDS = "屏幕录制";
const std::vector<AlbumPluginRowData> ALBUM_PLUGIN_DATA = {
    {"/DCIM/Camera", "相机", "Camera", "com." + BRAND_NAME + ".hmos.camera", "default-album-1", ".Camera"},
    {"/Pictures/Screenrecords", ALBUM_NAME_SCREEN_RECORDS, "Screenrecorder",
        "com." + BRAND_NAME + ".hmos.screenrecorder", "default-album-2", ".Screenshots"},
    {"/Pictures/Screenshots", "截图", "Screenshots", "com." + BRAND_NAME + ".hmos.screenshot",
        "default-album-2", ".Screenshots"},
    {"/", "根目录", "root directory", "", "", ".0"},
    {"/Pictures/hiddenAlbum", ".hiddenAlbum", "hiddenAlbum", "com.hidden.album", "default-album-4", ""},
    {"/Pictures/其它", "其它", "otherAlbum", "com.other.album", "", ""},
    {"/Pictures/mediatool", "mediatool", "mediatool", "com.mediatool.album", "", ""},
    {"/Download", "下载", "Downloads", "", "", ".Downloads"},
    {"/Imported", "已导入", "Imported", "", "", ".Imported"},
    {"/" + BRAND_NAME_UPPER_FIRST + "/MagazineUnlock", "杂志锁屏", "Magazine Unlock", "", "", ".MagazineUnlock"},
    {"/EditedOnlinePhotos", "编辑过的在线照片", "Edited online photos", "", "", ".EditedOnlinePhotos"},
    {"/CloudPicture", "下载中心", "Downloads", "", "", ".CloudPicture"},
    {"/Pictures", "图片", "Pictures", "", "", ".Pictures"},
    {"/Video", "视频", "Videos", "", "", ".Videos"},
    {"/Pictures/VideoEditor", "视频编辑", "Video Editor", "", "", ".VideoEditor"},
    {"/Pictures/Collage", "拼图", "Collages", "", "", ".Collage"},
    {"/Pictures/Browser", "浏览器", "Browser", "com.huawei.hmos.browser", "", ".Browser"},
    {"/Pictures/FromOtherDevices", "其他设备保存", "From other devices", "", "", ".FromOtherDevices"},
    {"/" + BRAND_NAME_UPPER_FIRST + "/preset", "预置图片", "Preset images", "", "", ".preset"},
    {"/" + BRAND_NAME_UPPER_FIRST + " Share",
        "华为分享", "" + BRAND_NAME_UPPER_FIRST + " Share",
        "com." + BRAND_NAME + ".hmos.instantshare", "", ".Huawei Share"},
    {"/bluetooth", "蓝牙分享", "Bluetooth", "", "", ".Bluetooth"},
    {"/Pictures/Meetime", "畅连", "MeeTime", "com." + BRAND_NAME + ".hmos.meetime", "", ".Meetime"},
    {"/Pictures/DoodleBoard", "白板", "Doodle Board", "", "", ".DoodleBoard"},
    {"/Pictures/Annotation", "批注", "Annotations", "", "", ".Annotation"},
    {"/360/images", "360清理大师", "360", "", "", ""},
    {"/10086Image", "中国移动手机营业厅", "10086", "com.chinamobile.cmcc", "", ""},
    {"/DCIM/1Videoshow", "乐秀视频", "1Videoshow", "", "", ""},
    {"/360Browser/download", "360浏览器", "360Browser", "com.qihoo.hms.browser", "", "", 0},
    {"/Download/360Browser", "360浏览器", "360Browser", "com.qihoo.hms.browser", "", ""},
    {"/Pictures/360Browser", "360浏览器", "360Browser", "com.qihoo.hms.browser", "", "", 0},
    {"/Pictures/Cooper", "库柏", "AIVideo", "", "", ".Cooper"},
    {"/DCIM/Alipay", "支付宝", "Alipay", "com.alipay.mobile.client", "", ""},
    {"/Pictures/Anjuke", "安居客", "anjuke", "com.anjuke.home", "", ""},
    {"/DCIM/ss_auto", "汽车之家", "autohomemain", "com.autohome.main", "", ""},
    {"/autohomemain", "汽车之家", "autohomemain", "com.autohome.main", "", "", 0},
    {"/autoprice/autoprice", "汽车报价大全", "autoprice", "", "", ""},
    {"/BaiduNetdisk", "百度网盘", "BaiduNetdisk", "", "", "", 0},
    {"/Download/BaiduNetdisk", "百度网盘", "BaiduNetdisk", "", "", ""},
    {"/Pictures/BaiduNews", "百度新闻", "BaiduNews", "", "", ""},
    {"/baofeng/p2p/download", "暴风影音", "baofeng", "", "", ""},
    {"/Pictures/ButterCamera", "黄油相机", "ButterCamera", "", "", ".ButterCamera"},
    {"/DCIM/CandyCam", "糖果照相机", "CandyCam", "", "", ""},
    {"/changbaPhoto", "唱吧", "changbaPhoto", "", "", ""},
    {"/netease/cloudmusic/网易云音乐相册", "网易云音乐", "cloudmusic", "com.netease.cloudmusic.hm", "", "", 0},
    {"/Download/netease/cloudmusic/网易云音乐相册", "网易云音乐", "cloudmusic", "com.netease.cloudmusic.hm", "", ""},
    {"/DCIM/comicool", "可米酷漫画", "comicool", "", "", ""},
    {"/CtripMedia/shortcut", "携程旅行", "CtripMedia", "", "", ""},
    {"/And""roidPotimizer/dds", "百度手机卫士", "dds", "", "", ""},
    {"/dianping", "大众点评", "dianping", "com.sankuai.dianping", "", ".dianping"},
    {"/DCIM/GroupRecorder", "大导演", "Director Mode", "", "", ".GroupRecorder"},
    {"/Pictures/dongqiudi", "懂球帝", "dongqiudi", "", "", ""},
    {"/ss_auto", "懂球帝", "dongqiudi", "", "", "", 0},
    {"/Pictures/ifeng/download_pic", "凤凰新闻", "ifeng download_pic", "com.ifeng.news.hmos", "", ".download_pic"},
    {"/Pictures/baidu/searchbox/downloads", "手机百度", "baidu downloads", "", "", ".downloads"},
    {"/baidu/searchbox/downloads", "手机百度", "baidu downloads", "", "", ".downloads", 0},
    {"/Hexin_image", "同花顺炒股票", "Hexin_image", "com.hexin.hmn.sjcg", "", "", 0},
    {"/Pictures/Hexin_image", "同花顺炒股票", "Hexin_image", "com.hexin.hmn.sjcg", "", ""},
    {"/HkImages", "好看锁屏", "HkImages", "", "", ""},
    {"/Pictures/huafans", "花粉俱乐郿", "huafans", "", "", ""},
    {"/" + BRAND_NAME_UPPER_FIRST + "/Themes/HWWallpapers", "锁屏壁纸", "HWWallpapers", "", "", ""},
    {"/icbcim/icbcim/usercard/webportrait", "工银融e联", "icbcim", "", "", ""},
    {"/Pictures/InstaMag", "拼立得", "InstaMag", "", "", ".InstaMag", 0},
    {"/DCIM/拼立得", "拼立得", "InstaMag", "", "", ".InstaMag"},
    {"/Pictures/iQIYI", "爱奇艺", "iQIYI", "com.qiyi.video.hmy", "", ".QIYIVideo", 0},
    {"/Pictures/com.qiyi.video", "爱奇艺", "iQIYI", "com.qiyi.video.hmy", "", ".QIYIVideo"},
    {"/QIYIVideo", "爱奇艺", "iQIYI", "com.qiyi.video.hmy", "", ".QIYIVideo", 0},
    {"/iReader/saveImage", "掌阅iReader", "iReader", "", "", ""},
    {"/DCIM/jdimage", "京东", "JDImage", "com.jd.hm.mall", "", ".jdimage", 0},
    {"/Pictures/JDImage", "京东", "JDImage", "com.jd.hm.mall", "", ".jdimage"},
    {"/tencent/karaoke/image", "全民K歌", "karaoke", "com.tencent.karaoke", "", "", 0},
    {"/DCIM/全民K歌", "全民K歌", "karaoke", "com.tencent.karaoke", "", ""},
    {"/KineMaster/Export", "巧影", "KineMaster", "", "", ""},
    {"/KuaiKan", "快看漫画", "KuaiKan", "", "", "", 0},
    {"/Pictures/Kuaikan", "快看漫画", "KuaiKan", "", "", ""},
    {"/KUWO_PIC", "酷我音乐", "KUWO_PIC", "", "", ""},
    {"/lvmama/downloadPic", "驴妈妈旅游", "lvmama", "", "", ""},
    {"/mafengwo", "马蜂窝", "mafengwo", "", "", "", 0},
    {"/Pictures/mafengwo/马蜂窝相册", "马蜂窝", "mafengwo", "", "", ""},
    {"/DCIM/Camera/maimai", "脉脉", "maimai", "", "", ""},
    {"/Pictures/meituan", "美团", "meituan", "com.sankuai.hmeituan", "", ".meituan"},
    {"/DCIM/mgtv", "芒果tv", "mgtv", "com.mgtv.phone", "", ""},
    {"/DCIM/mgtv", "芒果tv", "mgtv", "com.imgo.pad", "", ""},
    {"/DCIM/Camera/miaopai", "秒拍", "miaopai", "", "", ".miaopai"},
    {"/mirror_image", "Mirror Image", "Mirror Image", "", "", ""},
    {"/mitalkpics", "米聊", "mitalkpics", "", "", ""},
    {"/DCIM/Moji", "墨迹天气", "Moji", "com.moji.hmweather", "", ""},
    {"/MomanCamera", "魔漫相机", "MomanCamera", "", "", ".MomanCamera"},
    {"/DCIM/MomentCam", "魔漫相机绘图", "MomentCam", "", "", ""},
    {"/MomentCam/MomentCam_Drawing", "魔漫相机照片", "MomentCam_Drawing", "", "", ""},
    {"/MomentCam/MomentCam_Enotion", "魔漫相机表情", "MomentCam_Enotion", "", "", ""},
    {"/immomo/MOMO", "陌陌", "MOMO", "", "", ""},
    {"/MTTT", "美图贴贴", "MTTT", "", "", ".MTTT"},
    {"/MTXX", "美图秀秀", "MTXX", "com.meitu.meitupic", "", ".MTXX"},
    {"/MYXJ", "美颜相机", "MYXJ", "com.meitu.beautycam", "", ".MYXJ"},
    {"/Pictures/netease/newsreader", "网易新闻", "newsreader", "", "", ""},
    {"/Pictures/Note", "便签", "Note", "", "", ""},
    {"/Pictures/ogq", "OGQ壁纸", "OGQ", "", "", ""},
    {"/DCIM/Pindd/save_image", "拼多多", "pddPhotos", "", "", ""},
    {"/Photowonder", "百度魔图", "Photowonder", "", "", ".Photowonder"},
    {"/Pictures/Polarr", "泼辣修图", "Polarr", "", "", ""},
    {"/QieZi/pictures", "茄子快传", "QieZi", "", "", ""},
    {"/QieZi/video", "茄子快传", "QieZi", "", "", "", 0},
    {"/QieZi/video/拍摄", "茄子快传", "QieZi", "", "", "", 0},
    {"/qsbk/qiushibaike", "糗事百科", "qiushibaike", "", "", ""},
    {"/tencent/QQ_Favorite", "QQ表情", "QQ Favorite", "", "", ".QQ_Favorite"},
    {"/tencent/QQ_Images", "QQ", "QQ Images", "", "default-album-101", ".QQ_Images", 0},
    {"/Pictrures/QQ", "QQ", "QQ Images", "", "default-album-101", ".QQ_Images"},
    {"/QQBrowser/图片收藏", "QQ浏览器", "QQBrowser", "com.tencent.mtthm", "", ""},
    {"/QQMail", "QQ邮箱", "QQMail", "", "", ".QQMail", 0},
    {"/Download/QQMail", "QQ邮箱", "QQMail", "", "", ".QQMail"},
    {"/qqmusic/Qqlmagen", "QQ音乐", "qqmusic", "com.tencent.hm.qqmusic", "", "", 0},
    {"/Pictures/qqmusic/QQImage", "QQ音乐", "qqmusic", "com.tencent.hm.qqmusic", "", ""},
    {"/qqpim/pictures", "QQ同步助手", "qqpim", "", "", ""},
    {"/qsbk/video", "糗事百科的视频", "qsbkVideo", "", "", ""},
    {"/Quark/Download", "夸克浏览器", "Quark", "", "", ""},
    {"/tencent/QzonePic", "QQ空间", "Qzone", "", "", ".QzonePic"},
    {"/tencent/Qzone_Video", "QQ空间视频", "Qzone_Video", "", "", ""},
    {"/Pictures/Renren", "人人", "Renren", "", "", "", 0},
    {"/DCIM/Camera(人人视频)", "人人", "Renren", "", "", ""},
    {"/Pictures/Recover", "恢复", "Restore", "", "", ".Recover"},
    {"/sina/news/save", "新浪新闻", "sina news", "com.sina.news.hm.next", "", ".save", 0},
    {"/Pictures/sina", "新浪新闻", "sina news", "com.sina.news.hm.next", "", ".save"},
    {"/Secoo", "寺库奢侈品", "Secoo", "", "", ""},
    {"/ShareViaWLAN", "WLAN分享", "ShareViaWLAN", "", "", ".ShareViaWLAN"},
    {"/SohuDownload", "搜狐", "SohuDownload", "", "", ""},
    {"/shvdownload/video/SohuVideoGallery", "搜狐视频", "SohuVideoGallery", "", "", "", 0},
    {"/Pictures/SHVideoPic", "搜狐视频", "SohuVideoGallery", "", "", ""},
    {"/DCIM/SportsCamera", "小蚁运动相机", "SportsCamera", "", "", ""},
    {"/Pictures/suning", "苏宁易购", "suning", "", "", "", 0},
    {"/Pictures/suning.ebuy/image/share", "苏宁易购", "suning", "", "", ""},
    {"/Pictures/Tantan", "探探", "Tantan", "", "", ""},
    {"/Pictures/taobao", "淘宝", "taobao", "com.taobao.taobao4hmos", "", ".taobao"},
    {"/taobao", "淘宝", "taobao", "com.taobao.taobao4hmos", "", ".taobao", 0},
    {"/Pictures/TencentNews", "腾讯新闻", "TencentNews", "com.tencent.hm.news", "", ""},
    {"/Pictures/TencentReading", "天天快报", "TencentReading", "", "", ""},
    {"/tencent/TencentVideo/SavePic/doodle", "腾讯视频", "TencentVideo", "com.tencent.videohm", "", ""},
    {"/tencent/weibo/save", "腾讯微博", "TencentWeibo", "", "", ""},
    {"/xunlei/ThunderdownDB", "迅雷", "ThunderdownDB", "com.xunlei.thunder", "", ""},
    {"/tieba", "百度贴吧", "tieba", "", "", ".tieba"},
    {"/tencent/Tim_Images", "Tim", "Tim", "", "", ""},
    {"/DCIM/TmallPic", "天猫", "Tmall", "com.tmall.tmall4hmos", "", ""},
    {"/tuhulmg", "途虎养车", "tuhulmg", "com.tuhu.tuhuharmony", "", ""},
    {"/UCDownloads", "UC下载", "UCDownloads", "com.uc.mobile", "", ".UCDownloads", 0},
    {"/Download/UCDownloads", "UC下载", "UCDownloads", "com.uc.mobile", "", ".UCDownloads"},
    {"/UCDownloads/pictures", "UC下载", "UCDownloads", "com.uc.mobile", "", ".UCDownloads", 0},
    {"/UxinUsedCar", "优信二手车", "UxinUsedCar", "", "", ""},
    {"/viva_pics", "VIVA畅读", "viva_pics", "", "", ""},
    {"/Pictures/WeChat", "微信", "WeChat", "", "", ".WeiXin", 0},
    {"/Pictures/WeiXin", "微信", "WeChat", "", "", ".WeiXin"},
    {"/tencent/MicroMsg/WeChat", "微信", "WeChat", "", "", ".WeiXin", 0},
    {"/Tencent/MicroMsg/WeiXin", "微信", "WeChat", "", "default-album-102", ".WeiXin", 0},
    {"/sina/weibo/save", "微博", "Weibo", "com.sina.weibo.stage", "", ".weibo", 0},
    {"/sina/weibo/storage/photoalbum_save/weibo", "微博", "Weibo", "com.sina.weibo.stage", "", ".weibo", 0},
    {"/Pictures/weibo", "微博", "Weibo", "com.sina.weibo.stage", "", ".weibo"},
    {"/sina/weibo/weibo", "微博", "Weibo", "com.sina.weibo.stage", "default-album-103", ".weibo", 0},
    {"/微云保存的文件", "微云", "weiyun", "", "", ""},
    {"/tencent/weread/WeReadImage", "微信读书", "WeReadImage", "", "", ""},
    {"/WifiMasterKey/WiFiMasterPic", "WiFi万能钥匙", "WiFiMasterPic", "", "", ""},
    {"/Pictures/wuba", "58同城", "wuba", "com.wuba.life", "", ""},
    {"/Xender/image", "闪传的图牿", "XenderImage", "", "", ""},
    {"/Xender/video", "闪传的视频", "XenderVideo", "", "", ""},
    {"/xiaokaxiu", "小咖秀", "xiaokaxiu", "", "", ""},
    {"/YanXuan/image", "网易严选", "YanXuan", "", "", ""},
    {"/DCIM/youdao/photos", "有道词典", "youdao", "com.hm.youdao", "", ""},
    {"/zapya/photo", "快牙的图片", "zapyaPhoto", "", "", "", 0},
    {"/Pictures/zapya.photo", "快牙的图片", "zapyaPhoto", "", "", ""},
    {"/zapya/video", "快牙的视频", "zapyaVideo", "", "", ""},
    {"/Pictures/知乎", "知乎", "zhihu", "com.zhihu.hmos", "", ""},
    {"/DCIM/花椒", "花椒", "花椒", "", "", ""},
    {"/news_article", "今日头条", "今日头条", "com.ss.hm.article.news", "", ".news_article", 0},
    {"/funnygallery", "今日头条", "今日头条", "com.ss.hm.article.news", "", ".news_article", 0},
    {"/Pictures/news_article", "今日头条", "今日头条", "com.ss.hm.article.news", "", ".news_article"},
    {"/lightsky/ksp_download_video", "快视频", "快视频", "", "", ""},
    {"/DCIM/Sgame", "王者荣耀", "王者荣耀", "", "", ""},
    {"/Pictures/douyin", "抖音", "抖音", "com.ss.hm.ugc.aweme", "", ""},
    {"/DCIM/kugou/img", "酷狗音乐", "酷狗音乐", "com.kugou.hmmusic", "", "", 0},
    {"/Pictures/kugou", "酷狗音乐", "酷狗音乐", "com.kugou.hmmusic", "", ""},
    {"/Pictures/bili/screenshot", "哔哩哔哩", "bilibili", "yylx.danmaku.bili", "", "", 0},
    {"/Pictures/bili", "哔哩哔哩", "bilibili", "yylx.danmaku.bili", "", ""},
    {"/DCIM/net.xinhuamm.mainclient", "新华社", "新华社", "net.xinhuamm.xhshos", "", ""},
    {"/Pictures/人民网+", "人民网", "人民网", "cn.peopletech.peopleplusoh", "", ""},
    {"/Pictures/qqmusic/lyricposter", "QQ音乐", "QQMusic", "com.tencent.hm.qqmusic", "", ""},
    {"/DCIM/JianYing", "剪映", "剪映", "com.lemon.hm.lv", "", ""},
    {"/Pictures/喜马拉雅", "喜马拉雅", "喜马拉雅", "com.ximalaya.ting.xmharmony", "", ""},
    {"/Pictures/Trip", "携程旅行", "携程旅行", "com.ctrip.harmonynext", "", ""},
    {"/Pictures/du", "得物", "Du", "com.dewu.hos", "", ""},
    {"/DCIM/1688", "阿里巴巴", "阿里巴巴", "com.alibaba.wireless_hmos", "", ""},
    {"/Pictures/搜狐新闻", "搜狐新闻", "搜狐新闻", "com.sohu.harmonynews", "", ""},
    {"/DCIM/CamScanner", "扫描全能王", "扫描全能王", "com.intsig.camscanner.hap", "", ""},
    {"/DCIM/zhuanzhuan", "转转", "转转", "com.zhuanzhuan.hmoszz", "", ""},
    {"/Pictures/Keep", "Keep", "Keep", "com.gotokeep.hm.keep", "", ""},
    {"/DCIM/smartHome", "小翼管家", "小翼管家", "com.chinatelecom.esmarthome", "", ""},
    {"/DCIM/驾考宝典", "驾考宝典", "驾考宝典", "cn.mucang.hm.jiakao", "", ""},
    {"/Pictures/eastmoney", "东方财富", "东方财富", "com.eastmoney.hmn.berlin", "", ""},
    {"/DCIM/qinbaobao", "亲宝宝", "亲宝宝", "com.dw.btimeHarmony", "", ""},
    {"/Pictures/Trip", "智行火车票", "智行火车票", "com.suanya.harmonynext", "", "", 0},
    {"/DCIM/yiche", "易车", "易车", "com.yiche.autoeasyh", "", ""},
    {"/Pictures/jxedt", "驾校一点通", "驾校一点通", "com.jiaxiao.driveharmony", "", ""},
    {"/Pictures/Lark", "飞书", "飞书", "com.ss.feishu", "", ""},
    {"/Pictures/fenbi", "粉笔", "粉笔", "com.fenbi.gwy", "", ""},
    {"/DCIM/com.gtgj.view/pic/temp", "高铁管家", "高铁管家", "com.openet.gtgj", "", ""},
    {"/Pictures/nim", "大智慧", "大智慧", "com.gw.ohphone", "", ""},
};
} // namespace AlbumPlugin
} // namespace OHOS_Media
#endif // OHOS_MEDIA_ALBUM_PLUGIN_DATA_H