import express from "express"
import { model, connect, Error } from 'mongoose'
import {resolve} from 'path'

connect(process.env.MONGO_URI, { useNewUrlParser: true, useUnifiedTopology: true })

const Lamp = model('Lamp', {
    color: {
        type: [Number],
        default: [0, 0, 0]
    },
    message: {
        type: String,
        default: ''
    },
    tap: {
        type: Boolean,
        default: false
    },
    mac: {
        type: String,
        unique: true,
        required: true
    },
    bindedMac: String,
    lastUpdate: {
        type: Date,
        default: Date.now
    }
})

const app = express()
app.use(express.json())

app.get('/', (req, res) => {
    res.sendFile(resolve() + '/server/index.html')
})

app.get('/state', async (req, res) => {
    const { mac } = req.query
    const lamp = await Lamp.findOne({ mac })

    res.send(lamp)
    if (lamp.tap) {
        lamp.tap = false
        lamp.save()
    }
})

app.post('/state', async (req, res) => {
    try {
        const { color, message, tap, mac } = req.body
        const lamp = await Lamp.findOne({ mac })
        lamp.color = color
        lamp.message = message
        lamp.tap = tap
        lamp.lastUpdate = Date.now()
        lamp.save()
        res.status(200).send()
    } catch (e) {
        res
            .status(500)
            .send({ error: e.message })
    }
})

app.post('/tap', async (req, res) => {
    
    try {
        const { mac } = req.body
        const lamp = await Lamp.findOne({ mac })
        const otherLamp = await Lamp.findOne({ mac: lamp.bindedMac })
        otherLamp.tap = true
        otherLamp.lastUpdate = Date.now()
        otherLamp.save()
    } catch (e) {}
    res.status(200).send()
})

app.post('/bind', async (req, res) => {
    const { mac, bindedMac } = req.body
    const lamp = await Lamp.findOne({ mac })
    lamp.bindedMac = bindedMac
    lamp.save()
    const otherLamp = await Lamp.findOne({ mac: bindedMac })
    otherLamp.bindedMac = mac
    otherLamp.save()
    res.status(200).send()
})

app.post('/lamp', async (req, res) => {
    try {
        const { mac } = req.body
        const lamp = new Lamp({ mac })
        await lamp.save()
    } catch (e) {
        if (e.code === 11000) {
            res
                .status(200)
                .send()
        } else {
            res
                .status(500)
                .send({ error: e.message })
        }
    }
    res.status(200).send()
})

app.post('/binding', async (req, res) => {
    const { mac, bindedMac } = req.body
    const lamp = await Lamp.findOne({ mac })
    lamp.bindedMac = bindedMac
    lamp.save()
    res.status(200).send()
})

app.listen(process.env.PORT || 80, () => {
  console.log(`Server listening on port ${process.env.PORT || 80}`)
})