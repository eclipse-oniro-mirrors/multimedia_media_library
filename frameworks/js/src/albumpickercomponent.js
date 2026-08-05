/*
 * Copyright (c) 2024 Huawei Device Co., Ltd.
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
let __decorate = this && this.__decorate || function (e, o, t, i) {
    let n;
    let r = arguments.length;
    let l = r < 3 ? o : null === i ? i = Object.getOwnPropertyDescriptor(o,) : i;
    if ('object' === typeof Reflect && 'function' === typeof Reflect.decorate) {
        l = Reflect.decorate(e, o, t, i);
    } else {
        for (let s = e.length - 1; s >= 0; s--) {
            (n = e[s]) && (l = (r < 3 ? n(l) : r > 3 ? n(o, t, l) : n(o, t)) || l);
        }
    }
    return r > 3 && l && Object.defineProperty(o, t, l), l;
};
const display = requireNapi('display');
const cooperation_multi_name = ['Cooperation-multi', 'Cooperation'];
export class AlbumPickerComponent extends ViewPU {
    constructor(e, o, n, t = -1, i = void 0) {
        super(e, n, t);
        'function' === typeof i && (this.paramsGenerator_ = i);
        this.albumPickerOptions = void 0;
        this.onAlbumClick = void 0;
        this.onEmptyAreaClick = void 0;
        this.__albumPickerController = new SynchedPropertyNesedObjectPU(o.albumPickerController, this, 'albumPickerController');
        this.proxy = void 0;
        this.dpiFollowStrategy = SecurityDpiFollowStrategy.FOLLOW_UI_EXTENSION_ABILITY_DPI;
        this.__revokeIndex = new ObservedPropertySimplePU(0, this, 'revokeIndex');
        this.isPickerKilled = false;
        this.setInitiallyProvidedValue(o);
        this.declareWatch('albumPickerController', this.onChanged);
    }

    setInitiallyProvidedValue(e) {
        void 0 !== e.albumPickerOptions && (this.albumPickerOptions = e.albumPickerOptions);
        void 0 !== e.onAlbumClick && (this.onAlbumClick = e.onAlbumClick);
        void 0 !== e.onEmptyAreaClick && (this.onEmptyAreaClick = e.onEmptyAreaClick);
        this.__albumPickerController.set(e.albumPickerController);
        void 0 !== e.proxy && (this.proxy = e.proxy);
        const displayName = display.getDefaultDisplaySync().name;
        console.info(`displayName = ${displayName}`);
        if (cooperation_multi_name.includes(displayName)) {
            this.dpiFollowStrategy = SecurityDpiFollowStrategy.FOLLOW_HOST_DPI;
            console.info(`dpiFollowStrategy = ${this.dpiFollowStrategy}`);
        }
        if (e.revokeIndex !== undefined) { 
            this.revokeIndex = e.revokeIndex; 
        }
    }

    updateStateVars(e) {
        this.__albumPickerController.set(e.albumPickerController);
    }

    purgeVariableDependenciesOnElmtId(e) {
        this.__albumPickerController.purgeDependencyOnElmtId(e);
        this.__revokeIndex.purgeDependencyOnElmtId(e);
    }

    purgeVariableDependenciesOnElmtId(e) {
    }

    aboutToBeDeleted() {
        this.__albumPickerController.aboutToBeDeleted();
        this.__revokeIndex.aboutToBeDeleted();
        SubscriberManager.Get().delete(this.id__());
        this.aboutToBeDeletedInternal();
    }

    get albumPickerController() {
        return this.__albumPickerController.get();
    }

    get revokeIndex() { 
        if (this.__revokeIndex.get() === undefined) {
            this.__revokeIndex = new ObservedPropertySimplePU(0, this, 'revokeIndex');
        }
        return this.__revokeIndex.get(); 
    } 
 
 
    set revokeIndex(newValue) { 
        console.log(`albumpickercomponent set revokeindex ${newValue}`);
        return this.__revokeIndex.set(newValue); 
    }

    initialRender() {
        this.observeComponentCreation2(((e, o) => {
            Row.create();
            Row.height('100%');
        }), Row);
        this.observeComponentCreation2(((e, o) => {
            Column.create();
            Column.width('100%');
            Column.onVisibleAreaChange([0.0, 1.0], (isExpanding, currentRatio) => {
                if (this.isPickerKilled && currentRatio >= 0 && isExpanding) {
                    console.log(`AlbumPickerComponent isRevoke revokeIndex = ${this.revokeIndex}`);
                    this.revokeIndex++;
                    this.isPickerKilled = false;
                }
            });
        }), Column);
        this.observeComponentCreation2(((e, o) => {
            let n;
            let m;
            let i;
            SecurityUIExtensionComponent.create({
                parameters: {
                    errorRevokeIndex: this.revokeIndex,
                    'ability.want.params.uiExtensionTargetType': 'photoPicker',
                    targetPage: 'albumPage',
                    themeColorMode: null === (n = this.albumPickerOptions) || void 0 === n ? void 0 : n.themeColorMode,
                    filterType: null === (m = this.albumPickerOptions) || void 0 === m ? void 0 : m.filterType,
                    fontSize: null === (i = this.albumPickerOptions) || void 0 === i ? void 0 : i.fontSize,
                }
            },
            {
                dpiFollowStrategy: this.dpiFollowStrategy
            });
            SecurityUIExtensionComponent.height('100%');
            SecurityUIExtensionComponent.width('100%');
            SecurityUIExtensionComponent.onRemoteReady((e => {
                this.proxy = e;
                console.info('AlbumPickerComponent onRemoteReady');
            }));
            SecurityUIExtensionComponent.onReceive((e => {
                this.handleOnRecevie(e);
            }));
            SecurityUIExtensionComponent.onError(((error) => {
                console.info('AlbumPickerComponent onError');
                console.info('AlbumPickerComponent revokeIndex: ' + this.revokeIndex); 
                if (error.code === 100014) { 
                    console.log('AlbumPickerComponent is set isPickerKilled = true');
                    this.isPickerKilled = true; 
                }
            }));
        }), SecurityUIExtensionComponent);
        Column.pop();
        Row.pop();
    }

    onChanged() {
        let e;
        if (!this.proxy) {
            return;
        }
        let o = null === (e = this.albumPickerController) || void 0 === e ? void 0 : e.data;
        if (null == o ? void 0 : o.has('SET_FONT_SIZE')) {
            this.proxy.send({ fontSize: null == o ? void 0 : o.get('SET_FONT_SIZE') });
            console.info('AlbumPickerComponent onChanged: SET_FONT_SIZE');
        }
    }

    handleOnRecevie(e) {
        let o = e;
        let n = o.dataType;
        if ('selectAlbum' === n) {
            if (this.onAlbumClick) {
                let e = new AlbumInfo;
                e.uri = o.albumUri;
                e.albumName = o.albumName;
                this.onAlbumClick(e);
            }
        } else if ('emptyAreaClick' === n) {
            if (this.onEmptyAreaClick) {
                this.onEmptyAreaClick();
            }
        } else {
            console.info('AlbumPickerComponent onReceive: other case');
        }
        console.info('AlbumPickerComponent onReceive ' + n);
    }

    rerender() {
        this.updateDirtyElements();
    }
}

let AlbumPickerController = class {
    setFontSize(e) {
        this.data = new Map([['SET_FONT_SIZE', e]]);
        console.info('AlbumPickerComponent SET_FONT_SIZE ' + e);
    }
};
AlbumPickerController = __decorate([Observed], AlbumPickerController);

export class AlbumPickerOptions {
}

export class AlbumInfo {
}

export default { AlbumPickerComponent, AlbumPickerOptions, AlbumInfo, AlbumPickerController };