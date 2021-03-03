// list video quality:youtube-dl -F 'http://www.youtube.com/watch?v=P9pzm5b6FFY'

const data = require("./handmade.json")

let result = `@echo off
`

const target = data
  .filter(item => /Handmade Hero Day 130/.test(item.title))

for(let item of target) {
  result += `REM ${item.title}\n`
  result += `youtube-dl --no-playlist --proxy socks5://127.0.0.1:1086 -f mp4[height=720] "https://www.youtube.com${ item.url }"\n`
  result += "\n"
}

console.log(result)
