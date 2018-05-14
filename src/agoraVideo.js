var agoraVideo = new AgoraVideo;

agoraVideo.onJoinChannelSuccess = function (channel,  uid, elapsed){
    cc.log("[js]onJoinChannelSuccess，channel:%s,uid :%d, elapsed : %d !", channel, uid, elapsed);
    
    var event = new cc.EventCustom("localVideo");
    cc.eventManager.dispatchEvent(event );
}


agoraVideo.onLeaveChannel = function (totalDuration,  txBytes, rxBytes,txKBitRate,rxKBitRate,txAudioKBitRate,rxAudioKBitRate,txVideoKBitRate,rxVideoKBitRate,users,cpuTotalUsage,cpuAppUsage){
    cc.log("[js]onLeaveChannel，totalDuration:%s,utxBytes :%d, rxBytes : %d !\n", totalDuration, txBytes, rxBytes);
}

agoraVideo.onFirstRemoteVideoDecoded = function(uid,  width,  height, elapsed){
    cc.log("[js]onLeaveChannel，uid:%d,width :%d, height :%d ,elapsed: %d!\n", uid, width, height,elapsed);
    var event = new cc.EventCustom("remoteVideos");
    var msg =  {
        uid: uid,
        width : width,
        height : height,
        elapsed : elapsed,
    };
    event.setUserData( msg );
    cc.eventManager.dispatchEvent(event );
}

//enable
//  1: true
//  0: false
agoraVideo.onUserEnableVideo = function(uid, enable){
    cc.log("[js]onUserEnableVideo，uid:%d, enable: %d\n", uid, enable);

}
