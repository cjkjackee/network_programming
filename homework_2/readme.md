## homework2
- recvfrom 會return string 的長度, 沒有傳newline的時候記得在字串最後加NULL
- 不知道為何fopen有錯，所以換了個function
- 看看神奇的function stat()
- alarm(int)
    - 再過了int秒後才把signal function 裏面的handler的做的事情return
    - 再次呼叫alarm(0)會立即清掉上次的alarm
