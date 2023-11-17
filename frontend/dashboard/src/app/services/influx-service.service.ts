import { Injectable } from '@angular/core';
import { HttpClient, HttpHeaders } from '@angular/common/http';
import { environment } from 'src/enviroments/environment';

@Injectable({
  providedIn: 'root',
})
export class InfluxServiceService {
  constructor(private _http: HttpClient) {}

  queryData(_field: string, _range: string) {
    const url =
      'https://westeurope-1.azure.cloud2.influxdata.com/api/v2/query?org=dev';
    const headers = new HttpHeaders()
    .set('Authorization', `Token ${environment.influxToken}`)
    .set('Content-Type', 'application/vnd.flux');
  

    const body =
      `from(bucket: "BMWDATA")` +
      `|> range(start: -${_range})` +
      `|> filter(fn: (r) => r._measurement == "CarData")` +
      `|> filter(fn: (r) => r._field == "${_field}")` + // Here the _field variable is dynamically inserted
      `|> filter(fn: (r) => r.Car == "BMW E92")`;

    return this._http.post(url, body, {
      headers: headers,
      responseType: 'text',
    });
  }
}
