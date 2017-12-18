## Deepin Fprintd

Fingerprint backend for deepin desktop, contains the pam module.

### Dependencies


### Build dependencies

* [go-lib](https://github.com/linuxdeepin/go-lib)
* pam
* libfprint

### Runtime dependencies

* pam
* libfprint

## Installation


Build:
```
$ make GOPATH=/usr/share/gocode
```

Or, build through gccgo
```
$ make GOPATH=/usr/share/gocode USE_GCCGO=1
```

Install:
```
sudo make install
```

## Getting help

Any usage issues can ask for help via

* [Gitter](https://gitter.im/orgs/linuxdeepin/rooms)
* [IRC channel](https://webchat.freenode.net/?channels=deepin)
* [Forum](https://bbs.deepin.org/)
* [WiKi](https://wiki.deepin.org/)

## Getting involved

We encourage you to report issues and contribute changes.

* [Contribution guide for developers](https://github.com/linuxdeepin/developer-center/wiki/Contribution-Guidelines-for-Developers-en). (English)
* [开发者代码贡献指南](https://github.com/linuxdeepin/developer-center/wiki/Contribution-Guidelines-for-Developers) (中文)

## License

Deepin fprintd is licensed under [GPLv3](LICENSE).
