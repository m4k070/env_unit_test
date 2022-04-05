import { Chart, ChartData, registerables } from 'chart.js';

let labels: string[] = [];
let data: ChartData = {
	labels: labels,
	datasets: [
		{ type: 'line', label: '気温', data: [], yAxisID: 'temp', borderColor: 'rgba(255, 0, 0, 0.5)', },
		{ type: 'line', label: '湿度', data: [], yAxisID: 'hum', borderColor: 'rgba(0, 0, 255, 0.5)', },
		{ type: 'line', label: '気圧', data: [], yAxisID: 'press', borderColor: 'rgba(0, 255, 0, 0.5)', },
	]
};

let myChart: Chart | null = null;

const update = async () => {
	const resp = await fetch("http://192.168.4.1/env", {
		method: 'GET',
	});
	const env = await resp.json();
	const d = new Date();
	data.labels?.push(d.toISOString());
	data.datasets[0].data.push(env.Temperture);
	data.datasets[1].data.push(env.Humidity);
	data.datasets[2].data.push(env.Pressure);
	//console.log(config);
	myChart?.update();
};

window.onload = function (e) {
	Chart.register(...registerables);
	//console.log('onload');
	update();
	myChart = new Chart('myChart', {
		type: 'line',
		data: data
	});

	setInterval(update, 10000);
};
