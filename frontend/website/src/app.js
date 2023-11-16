const http = require('http')
require("dotenv").config(); // install dependency with npm install dotenv to import .env variables
const influxConnect = require("./query"); //import query

const app = http.createServer(async (req, res) => {
  const url = req.url
  console.log(url)
  if (url === '/charts') {
    const data = await influxConnect()
    res.writeHead(200, { 'Content-Type': 'application/json', 'Access-Control-Allow-Origin': '*' })
    res.end(JSON.stringify(data))
  }
}
)

const PORT = process.env.PORT || 8080

app.listen(8080, () => {
  console.log(`Server is listening on port ${PORT}`)
})