const express = require('express')
const WebSocket = require('ws')
const path = require('path')

const port = 80
const wsport = 81

/*
  So, the webserver just dishes up the static page
  and the websocket needs to feed fake data
*/
const app = express()
app.get('/', (req, res) => {
  const opts = {
    root: path.join(__dirname),
  }
  res.sendFile('test.html', opts)
})
app.listen(port, () => {
  console.log(`Example app listening at http://localhost:${port}`)
})

const wss = new WebSocket.Server({ port: wsport })
wss.on('connection', function connection(ws) {
  ws.on('message', function incoming(message) {
    console.log('received: %s', message)
  })
  ws.on('close', function closedForSomeReason() {
    console.log('websocket closed for some reason')
  })
})

const fakeData = {
  cat: 'hat',
  tea: 'pot',
  value: 18.00,
}

const meta = {
  rising: true,
  min: 16.00,
  max: 49.00,
}

function randBetween(min, max) {
  return Math.random() * (max - min) + min
}

/*
 * Fake the temperature rising and falling to give a nice representative chart
 */
function updateTemperature() {
  const diff = randBetween(-0.6, 1.8)
  if (meta.rising) {
    fakeData.value += diff
    if (fakeData.value >= meta.max) {
      meta.rising = false
    }
  } else {
    fakeData.value -= diff
    if (fakeData.value <= meta.min) {
      meta.rising = true
    }
  }
}

function fakeSomeStuff() {
  updateTemperature()
  wss.clients.forEach((client) => {
    if (client.readyState === WebSocket.OPEN) {
      client.send(JSON.stringify(fakeData))
    }
  })
}

setInterval(fakeSomeStuff, 1000)

