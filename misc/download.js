const data = require("./handmade.json")

let result = `@echo off
`

const target = data
  .filter(item => /Handmade Hero Day 09([0-9])/.test(item.title))

for(let item of target) {
  result += `youtube-dl --proxy socks5://127.0.0.1:1086 -r 1M -f best https://www.youtube.com${ item.url }
`
}

console.log(result)
