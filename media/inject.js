
const fs = require('fs')
const path = require('path')

const hf = path.join(__dirname, 'test.html')
const html = fs.readFileSync(hf).toString('utf8')
const sf = path.join(__dirname, '..', 'src', 'main.cpp')
let s = fs.readFileSync(sf).toString('utf8')
s = s.replace(/R"=====\(.*\)====="/s, 'R"=====(\n' + html + '\n)====="')
fs.writeFileSync(sf, s)
