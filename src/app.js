var _remoteVideoSprite = new Map();

var HelloWorldLayer = cc.Layer.extend({

    roomInput: null,
    sprite:null,
    _localVideoSprite:null,
                                      
    _debugremoteVS:null,
                                    
    ctor:function () {
        //////////////////////////////
        // 1. super init first
        this._super();
        cc.log("new LoginLayer\n");
        /////////////////////////////
        // 2. add a menu item with "X" image, which is clicked to quit the program
        //    you may modify it.
        // ask the window size
        var size = cc.winSize;

        /////////////////////////////
        // 3. add your codes below...
        // add a label shows "Hello World"
        // create and initialize a label
        var helloLabel = new cc.LabelTTF("Hello Agora Video ", "Arial", 30);
        // position the label on the center of the screen
        helloLabel.x = size.width / 2;
        helloLabel.y = size.height / 2 + 300;
        // add the label as a child to this layer
        this.addChild(helloLabel, 5);

        // add "HelloWorld" splash screen"
//        this.sprite = new cc.Sprite(res.HelloWorld_agora);
//        this.sprite.attr({
//            x: size.width / 2,
//            y: size.height / 2
//        });
//        this.addChild(this.sprite, 0);
        
        this.loadUI();
        this.addVideoEventListener();

        return true;
    },

    loadUI: function () {
                                      
        var leftPadding = 50;
      
        var visibleSize = cc.director.getVisibleSize();
        var WinSize = cc.director.getWinSize();
        cc.log("WinSize.width :%d, visibleSize.width: %d\n", WinSize.width, visibleSize.width);
                                      
        // var labelID = cc.LabelTTF("输入用户ID", "Arial", 80);
        // labelID.setPosition(labelX, 600);
        // labelID.setAnchorPoint(0, 0);
        // this.addChild(labelID);

        this.roomInput = cc.EditBox(cc.size(200, 60), "res/TextBox.png");
        this.roomInput.setTextHorizontalAlignment(cc.TEXT_ALIGNMENT_LEFT);
        this.roomInput.setInputMode(cc.EDITBOX_INPUT_MODE_SINGLELINE);
        this.roomInput.setAnchorPoint(0, 0);
        this.roomInput.setPosition(leftPadding , visibleSize.height - 100);
        this.addChild(this.roomInput);
        this.roomInput.setString("666")

        var joinBtn = ccui.Button();
        joinBtn.setTitleFontSize(20);
        joinBtn.setPosition(leftPadding*3, visibleSize.height -160);
        joinBtn.scale = 0.75
        joinBtn.setTouchEnabled(true)
        joinBtn.loadTextures("res/Button.png", "res/ButtonPressed.png", "res/ButtonPressed.png")
        joinBtn.setTitleText("JoinChannel");
        this.addChild(joinBtn)

        var myThis  = this;
        joinBtn.addClickEventListener(function () {
            cc.log("joinBtn");
            cc.log("ver : %s",agoraVideo.setParameters("{\"che.video.enableRemoteViewMirror\":true}"));
            myThis.joinChannel();
        });

        var leaveBtn = ccui.Button();
        leaveBtn.setTitleFontSize(20);
        leaveBtn.setPosition(leftPadding*3, visibleSize.height -250);
        leaveBtn.scale = 0.75
        leaveBtn.setTouchEnabled(true)
        leaveBtn.loadTextures("res/Button.png", "res/ButtonPressed.png", "res/ButtonPressed.png")
        leaveBtn.setTitleText("leaveChannel");
        this.addChild(leaveBtn)
        leaveBtn.addClickEventListener(function () {
            cc.log("leaveBtn !"),
            myThis.leaveChannel();
        });                           

//        var btnClose = ccui.Button();
//        btnClose.setTitleFontSize(10);
//        btnClose.setTouchEnabled(true)
//        btnClose.setPosition(visibleSize.width -15, 15);
//        btnClose.loadTextures("res/CloseNormal.png", "res/CloseSelected.png", "res/CloseNormal.png")
//        this.addChild(btnClose );
//        btnClose.addClickEventListener( function () {
//
//            agoraVideo.leaveChannel();
//            myThis.clearVideoSprites();
//            agoraVideo.appExit();
//        }
                                      
    },
    
    joinChannel:function(){
        g_roomName = this.roomInput.getString();
        cc.log("roomName:%s\n", g_roomName);

        //agoraVideo
        /*!
         *  @param g_roomName   Channel name
         *  @param uid  user id.
         *  @param  videoEnabled   1: enable  video , 0: disable video
         *  @param  videoMode   one of VIDEO_PROFILE  in agora_cocos2dx.h
         *  @param  info   ""
         *  @return errCode
         */
        var uid = 0;
        var videoEnabled = 1;
        var videoMode = 35;
        var info ="";
    
        var errCode = agoraVideo.joinChannel(g_roomName, uid, videoEnabled, videoMode, "");
        if( 0  === errCode){
            cc.log("logIn Successfully !\n");
        }else {
            cc.log("logIn Failed，errCode：%d\n",  errCode);
        }
    },

                                      
    leaveChannel:function(){
        var errCode = agoraVideo.leaveChannel();
        if( 0 == errCode){
            cc.log("leaveChannel is called Successfully !\n");
        }else {
            cc.log("leaveChannel is called Failed，errCode：%d\n",  errCode);
        }

        this.clearVideoSprites()                   
    },
    
    clearVideoSprites:function(){
                                      
        if (null != _localVideoSprite){
            this.removeChild(_localVideoSprite)
            this._localVideoSprite = null;
        }

        if (null != _remoteVideoSprite){
            cc.log("_remoteVideoSprite.size : " + _remoteVideoSprite.size);
                        
            for (var [key, value] of _remoteVideoSprite) {
                console.log(key + ' = ' + value);
                this.removeChild(value);
                _remoteVideoSprite.delete(key);
            }
                                      
            cc.log("_remoteVideoSprite.size : " + _remoteVideoSprite.size);
        }                             
     },
                                    

    addVideoEventListener:function(){
        var mythis = this;
        var visibleSize = cc.director.getVisibleSize();
                                      
        var localVideo = cc.EventListener.create({
            event:cc.EventListener.CUSTOM,
            eventName:"localVideo",
            callback:function (event){
                cc.log("localVideo display ！\n");
                _localVideoSprite = agoraVideo.getLocalSprite();
                _localVideoSprite.setPosition(cc.p(visibleSize.width/2, visibleSize.height/2));
                mythis.addChild(_localVideoSprite);
            }
        });
                                      
        cc.eventManager.addListener(localVideo, 1);
        var localVideo = cc.EventListener.create({
            event:cc.EventListener.CUSTOM,
            eventName:"remoteVideos",
            callback:function (event){
                var msg = event.getUserData();
                                                
                _remoteVideoSprite.set(msg.uid, agoraVideo.getRemoteSprites(msg.uid));
                _remoteVideoSprite.get(msg.uid).setPosition(cc.p((visibleSize.width/2 + 330), visibleSize.height/2));
                mythis.addChild(_remoteVideoSprite.get(msg.uid));
            }
        });
        cc.eventManager.addListener(localVideo, 1);
    },
            
});

var HelloWorldScene = cc.Scene.extend({
    onEnter:function () {
        this._super();
        var layer = new HelloWorldLayer();
        this.addChild(layer);
    }
})