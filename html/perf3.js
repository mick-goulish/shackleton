/*
 -
 - Licensed to the Apache Software Foundation (ASF) under one
 - or more contributor license agreements.  See the NOTICE file
 - distributed with this work for additional information
 - regarding copyright ownership.  The ASF licenses this file
 - to you under the Apache License, Version 2.0 (the
 - "License"); you may not use this file except in compliance
 - with the License.  You may obtain a copy of the License at
 -
 -   http://www.apache.org/licenses/LICENSE-2.0
 -
 - Unless required by applicable law or agreed to in writing,
 - software distributed under the License is distributed on an
 - "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
 - KIND, either express or implied.  See the License for the
 - specific language governing permissions and limitations
 - under the License.
 -
*/
angular.module('qpid.proton.timings', ['ngAnimate', 'ui.bootstrap', 'ngStorage'])
.controller('QPTPController', function ($scope, $http, $localStorage) {

	// This code uses angular's $watch feature to magically trigger functions when a value is changed.
	// So when the $scope.date is changed in the interface, the $scope.$watch('selectors.date') handler is automatically called.
	// This in turn will get the list of tests under the date and change the scope.test...
	// wich will trigger the testChanged() hander, and so on.
	// This cascades for the scope.selector variables date, test, language, and client.

	// the contents of the json file that describes the directory structure
    var results;
	var descriptions;   // from results/descriptions.json - info about each test
	$scope.charts = [];
	var c3Charts = [];

	// these are wrapped in an object because they are used in a tab.
	// see http://stackoverflow.com/questions/27513462/angularjs-ngmodel-doesnt-work-inside-a-ui-bootstrap-tabset
    $scope.selectors = {
        date:       undefined,
        test:       {prompt: 'Test', curVal: undefined, allVals: []},
        language:   {prompt: 'Language', curVal: undefined, allVals: []},
        client:     {prompt: 'Application', curVal: undefined, allVals: []}
    }

	// called by the change handlers when the variable they are watching is changed
	// This will get the list of values for the next selector, set the next selectors current value
	// and ensure the next selector's change handler is triggered.
    var sectionChanged = function (newVal, thisSelector, nextSelector, root, nextChanged, save) {
		// save the current value
		if (save)
            $localStorage[thisSelector] = newVal;

		if (!root)
			$scope.selectors[nextSelector].allVals = [];
		else
			// get the list of all values for the selector
            $scope.selectors[nextSelector].allVals = Object.keys(root);
        // get the prefered value. if the old value is in the allVals list, use it. otherwise use the 1st val in the list
        var prefered = getScopeVariable(nextSelector, $scope.selectors[nextSelector].curVal, $scope.selectors[nextSelector].allVals);
		// if the prefered val is the same as the current val, manually trigger the next handler
        if ($scope.selectors[nextSelector].curVal === prefered)
            nextChanged(prefered);
        else
	        // set the new value which will trigger the next handler
	        $scope.selectors[nextSelector].curVal = prefered;
    }

	// called when the value of $scope.selectors.date changed
	var dateChanged = function (newVal) {
        if (!newVal | !results) return;
        var parts = dateParts(newVal);
		sectionChanged(newVal, 'date', 'test', results[parts[0]][parts[1]][parts[2]], testChanged, false);
    }

	// called when the value of $scope.selectors.test.curVal changed
    var testChanged = function (newVal) {
        if (!newVal | !results) return;
        var parts = dateParts($scope.selectors.date);
		var save = true;
		if ($scope.overrideTest) {
			newVal = $scope.overrideTest;
			save = false;
			if (!results[parts[0]][parts[1]][parts[2]][newVal]) {
				// the requested test is not available on the current date
                var priorDate = new Date();
                priorDate.setDate($scope.selectors.date.getDate() - 1);
				var firstDate = new Date('11/03/2015 00:00:00')
				if (priorDate >= firstDate) {
					$scope.selectors.date = priorDate;
				}
				return;
			}
		}
		sectionChanged(newVal, 'test', 'language', results[parts[0]][parts[1]][parts[2]]
													[newVal], languageChanged, save);
	}
	// called when the value of $scope.selectors.language.curVal changed
    var languageChanged = function (newVal) {
        if (!newVal | !results) return;
        var parts = dateParts($scope.selectors.date);
		sectionChanged(newVal, 'language', 'client', results[parts[0]][parts[1]][parts[2]]
													[$scope.selectors.test.curVal][newVal], clientChanged, true);
    }

	// create a list of filenames that we want to fetch
	var getLastData = function (dates, test, language, client) {
		var allData = [];
		var parts;
		var startdate = dates.length >= 5 ? dates.length - 5 : 0;
		for (var i=startdate; i < dates.length; ++i) {
			parts = dateParts(dates[i]);
			if (results[parts[0]][parts[1]][parts[2]][test] &&
				results[parts[0]][parts[1]][parts[2]][test][language] &&
				results[parts[0]][parts[1]][parts[2]][test][language][client]) {

		        var charts = results[parts[0]][parts[1]][parts[2]]
	                                        [test][language]
	                                        [client];
				var genSrc = function (chartIndex, latency) {
					if (chartIndex == -1)
						return null;
					var src = ['results', 'tests',
	                         parts[0], parts[1], parts[2],
	                         test,
	                         language,
	                         client, charts[chartIndex]].join('/');
		            allData.push({src: src, date: dates[i], latency: latency});
				}
				var tp_report = charts.indexOf("tp_report.txt");
				var l_report = charts.indexOf("l_report.txt");
				genSrc(tp_report, false);
				genSrc(l_report, true);

			}

		}
		return allData;
	}

	// called when the value of $scope.selectors.client.curVal changed
    var clientChanged = function (newVal) {
        if (!newVal | !results) return;

		if (!$scope.overrideClient)
	        $localStorage['client'] = newVal;

		var gotAllSamples = function (allSamples) {
			// the data is now homogonized. call the chart factory to make the appropriate chart
			$scope.charts = [];
			chartFactory(allSamples,
				$scope.selectors.language.curVal,
				$scope.selectors.test.curVal,
				newVal,
				$scope.selectors.date,
				$scope.charts);
			// we got here asynchronously after all the data was loaded, so we need to manually apply any scope changes
			$scope.$apply();

			// wait for the charts to appear on the page
	        showCharts($scope.charts, 'id');
		}
		var dateIndex = lastDates.map(Number).indexOf(+$scope.selectors.date)
		var dates = lastDates.slice(Math.max(dateIndex-4, 0), dateIndex+1)

		getAllSamples(dates,
                        $scope.selectors.test.curVal,
                        $scope.selectors.language.curVal,
                        newVal, gotAllSamples );
	}

	var getAllSamples = function (dates, test, language, client, callback) {
		var dataFiles = getLastData(dates, test, language, client);
	    var q = queue();
	    dataFiles.forEach ( function (file) {
	        q.defer(d3.tsv, file.src);
	    })
	    q.awaitAll( function (error, results) {
			if (error) {
				return;
			}

			var allSamples = [];
			results.forEach( function (result, fileIndex) {
				var daySamples = {date: dataFiles[fileIndex].date, results: result, latency: dataFiles[fileIndex].latency};
				allSamples.push(daySamples);
			})
			callback(allSamples);
	    })
    }

	$scope.zoomChart = function (chart) {
		if (chart.zoomed) {
			chart.c3.axis.min({y: 0})
		}
		else {
			chart.c3.axis.min(chart.min)
		}
		if (chart.pinned)
			savePinned();
	}
	var getMin = function (c3) {
		var m = {y: undefined};
		var data = c3.data();
		data.forEach( function (d) {
			var min = d3.min(d.values, function (o) { return o.value })
			if (!m.y || min < m.y)
				m.y = min;
		})
		return m;
	}

    var showCharts = function (charts, key) {
        // ensure the div for our chart is loaded in the dom
		charts.forEach( function (chart) {
			if (!$scope.overrideClient || chart.dash ) {
		        var div = angular.element("#"+chart[key]);
		        if (!div.width()) {
		            setTimeout(showCharts, 100, charts, key);
		            return;
		        }
			}
		})
		charts.forEach( function (chart) {
			if (!chart.c3) {
				var xFormat = chart.xFormat ? chart.xFormat : '.3s'
				var c3Chart = gen_a_chart(chart[key], chart.columns, chart.yLabel, chart.yFormat, xFormat, chart.tickVals, chart.callback)
				chart['c3'] = c3Chart;
				chart['min'] = getMin(c3Chart);
			}
			if (chart.zoomed)
				$scope.zoomChart(chart);
		})
    }

	var gen_a_chart = function (id, columns, yLabel, yFormat, xFormat, tickVals, callback) {
		var chart = c3.generate({
			bindto: '#'+id,
			size: { width: $scope.overrideTest ? 800 : 1024, height: 300 },
		    data: {
		        x:       'x',
		        columns: columns,
		        type: 'line'
		    },
		    axis: {
                  x: {
		            type: 'indexed',
		            tick: {
		                format: d3.format(xFormat),
		                values: tickVals
		            }
                  },
		        y: {
		            tick: {
		                format: d3.format(yFormat),
					},
		            label: {
		                text: yLabel,
		                position: 'outer-middle'
		            }
		        }
		    },
		    legend: {
		        position: 'bottom'
		    },
		    grid: {
		      x: {
		        show: true
		      },
		      y: {
		        show: true
		      }
		    },
		    tooltip: {
		        format: {
		            title: function (d) { return callback(d) }
		        }
		    }
		});
		return chart;
	}

	var chartFactory = function (allSamples, language, test, client, date, pushTo, measurement) {

		var client_override = function (o) {
			if (typeof o === 'object') {
				if (angular.isDefined(o[client]))
					o = o[client]
				else if (angular.isDefined(o['default']))
					o = o['default']
				else
					o = 'undefined'
			}
			return o;
		}
		var json_chart = function (charts, xaxis) {
			var xColumn = client_override(xaxis.column);
			var divisor = 1;
			if (angular.isDefined(xaxis.divisor))
				divisor = client_override(xaxis.divisor)
			var columns = [];
			columns[0] = ['x'];

			var values = [];
			charts.forEach(function (chart, i) {
				values[i] = [];
				values[i][0] = columns[0];
			})
			var initCols = true;
			allSamples.forEach ( function (daySample, dayIndex) {
				var date = daySample.date;
				var results = daySample.results;
				var subDate = date.toDateString().substr(4);
				charts.forEach(function (chart, i) {
					values[i][dayIndex+1] = [subDate];
				})
				results.forEach( function (result) {
					if (initCols) {
						if (angular.isDefined(result[xColumn]))
							columns[0].push ( result[xColumn] / divisor )
					}
					charts.forEach( function (chart, i) {
						if (chart.column1) {
							values[i][dayIndex+1].push( result[chart.column1] * result[chart.column2] )
						} else {
							var column = client_override(chart.column)
							if (angular.isDefined(result[column]))
								values[i][dayIndex+1].push( result[column] )
						}
					})
				})
				initCols = false;
			})
			var ret = {};
			charts.forEach ( function (chart, i) {
				ret[chart.id] = values[i]
			})
			return ret;

		}

		var meta = descriptions[test];
		var columns = json_chart(meta.charts, meta.axis.x)

		var tipCallback = function (d) {
			var val = d
			if (meta.tooltip.format != "")
				val = d3.format(meta.tooltip.format)(d)
			return client_override(meta.tooltip.pre) + val + client_override(meta.tooltip.post);
		}

		var measureChart = null
		meta.charts.forEach (function (chart) {
			chart.c3 = undefined;   // remove any prior generated chart
			chart.xLabel = client_override(meta.axis.x.label);
			chart.columns = columns[chart.id];
			chart.tickVals = angular.isDefined(meta.axis.x.ticks) ? meta.axis.x.ticks : null;
			chart.callback = tipCallback;
			decorateChartDef(chart, chart.id, language, test, client, date);
			if (chart.id === measurement)
				measureChart = chart
		})
		if (measurement && measureChart) {
			pushTo.push (measureChart)
		} else {
			meta.clients[client].forEach( function (graph) {
				pushTo.push(meta.charts.filter(function (chart) {
					return chart.id === graph
				})[0])
			})
		}
	}

	$scope.translateId = function (id) {
		if (id == 'cpu') return 'CPU';
		if (id == 'rss') return 'Memory';
		if (id == 'throughput') return 'Throughput';
		if (id == 'latency') return 'Latency';
		return id;
	}
	/* ---------- Some utility function -------------- */
	var decorateChartDef = function (chartDef, key, language, test, client, date) {
		var zoomed;
		var pinned = false;
		date = new Date(date);
		chartDef['date'] = date;
		var uuid = language+test+client+key+date.toISOString().substring(0,10);
		uuid = uuid.replace(/[^\w]/gi, '')
		chartDef['uuid'] = uuid;

		var pinChart = findPinned(uuid);
		if (pinChart) {
			pinned = true;
			zoomed = pinChart.zoomed;
		}

		chartDef['zoomed'] = zoomed;
		chartDef['pinned'] = pinned;
		chartDef['language'] = language;
		chartDef['test'] = test;
		chartDef['client'] = client;
	}


	// return a prefered value to be used for a scope variable
    var getScopeVariable = function (key, oldVal, possibleVals) {
        // get the value from storage if it hasn't been initialized yet
        var preferedVal = !oldVal ? $localStorage[key] : oldVal;
		// if the prefered value is in the new list of possible values
		if ($scope.overrideClient && key === 'client')
			preferedVal = $scope.overrideClient;
		if ($scope.overrideTest && key === 'test')
			preferedVal = $scope.overrideTest;

        if (possibleVals.indexOf(preferedVal) > -1) {
            return preferedVal;
        } else {
            return possibleVals[0];
        }
    }

	// split a date into strings for yyyy, mm, dd
    var dateParts = function (d) {
        var yyyy = d.getFullYear() + "";
        var mm = "0" + (d.getMonth() + 1);
        var dd = "0" + d.getDate();
        yyyy + (mm[1]?mm:"0"+mm[0]) + (dd[1]?dd:"0"+dd[0]);
        return [yyyy, mm.slice(mm.length-2), dd.slice(dd.length-2)];
    }

	/* ---------- The following are for the angular-ui date popup -------------- */
    $scope.open = function($event) {
        $scope.status.opened = true;
    };

    // Disable any dates for which we don't have data
    $scope.disabled = function(date, mode) {
        var parts = dateParts(date);
		return !results[parts[0]][parts[1]][parts[2]];
    };

    // options for the date popup
    $scope.dateOptions = {
        formatYear: 'yy',
        showWeeks: false
    };
    $scope.formats = ['dd-MMMM-yyyy', 'yyyy/MM/dd', 'dd.MM.yyyy', 'shortDate'];
    $scope.format = $scope.formats[0];
    $scope.status = { opened: false };

	var findPinned = function (uuid) {
		var pin = $scope.pinned.filter( function (pinned) {
			return pinned.uuid === uuid;
		})
		if (pin.length)
			return pin[0];
		// else return is undefined
	}
	var getBaseProps = function (chart) {
		return {
			id:         chart.id,
			date:       chart.date,
			language:   chart.language,
			test:       chart.test,
			client:     chart.client,
			uuid:       chart.uuid,
			zoomed:     chart.zoomed,
			pinned:     chart.pinned
		}
	}
	var copyChart = function (src) {
		copy = {};
		for (var key in src) {
			copy[key] = src[key]
		}
		return copy;
	}

	var savePinned = function () {
		// remove the runtime properties from the pinned charts and save only the important bits
		var pinned = [];
		$scope.pinned.forEach( function (chart) {
			pinned.push(getBaseProps(chart))
		})
		$localStorage['pinned2'] = angular.toJson(pinned);
	}
	/* ---------- save/maintain the pinned and selected lists -------------- */
	// a chart was just pinned/unpinned, remove it from the list if it's there, otherwise add it
	$scope.pinChart = function (chart, $event) {
		var found = $scope.pinned.some( function (pin, i) {
			if (pin.uuid == chart.uuid) {
				chart.pinned = false;
				$scope.pinned.splice(i, 1);
				// if this chart is on the charts page, set it's pinned to false
				$scope.charts.some( function (c) {
					if (c.uuid === pin.uuid) {
						c.pinned = false;
						return true;
					}
					return false;
				})
				return true;    // stop looping, sets found to true
			}
			return false;
		});
		if (!found) {
			chart.pinned = true;
			var copy = copyChart(chart);
			copy.c3 = undefined;    // force chart to recreate c3 object
			$scope.pinned.push(copy);
		}
		savePinned();
	}

	// a tab was just clicked. save the states
	$scope.saveTabState = function (charts) {
		$scope.tabState.charts = charts;
		$scope.tabState.saved = !charts;
		$localStorage['tabState'] = angular.toJson($scope.tabState);
	}

	var showSavedCharts = function () {
		var fetch = 0;
		var received = 0;
			// count the number of chart we will need to fetch
		$scope.pinned.forEach( function (chart, chartIndex) {
			if (!chart.columns)
				++fetch;
		})

		$scope.pinned.forEach( function (chart) {
			var gotAllSamples = function (allSamples) {
				// the data is now homogonized. call the chart factory to make the appropriate chart
				var newChart = [];
				chartFactory(allSamples,
					chart.language,
					chart.test,
					chart.client,
					chart.date,
					newChart, chart.id);
				for (var key in newChart[0]) {
					chart[key] = newChart[0][key]; // adds columns, title, etc.
				}
				++received;
				if (received == fetch) {
					// we got here asynchronously after all the data was loaded, so we need to manually apply any scope changes
			        $scope.$apply();

			        // wait for the charts to appear on the page
			        showCharts($scope.pinned, 'uuid');
				}
			}

			if (!chart.columns) {
				var dates = [];
				var date = new Date(chart.date);
				dates.push(date);
				for (var i=1; i<5; ++i) {
					var past = new Date(date.getTime());
					past.setDate(date.getDate() - i);
					dates.unshift(past);
				}
				getAllSamples(dates,
	                chart.test,
	                chart.language,
	                chart.client,
	                gotAllSamples);
			}
		})
		if (!fetch)
	        // wait for the charts to appear on the page
	        showCharts($scope.pinned, 'uuid');

	}

	$scope.$watch('tabState.saved', function (newVal) {
        if (!newVal | !results) return;

		showSavedCharts();
	})

	// initialize which tab should be active
	var tabState = angular.fromJson($localStorage['tabState']);
	$scope.tabState = tabState ? tabState : {charts: true, saved: false};

	/* ---------- setup the watchers -------------- */
    $scope.$watch('selectors.date',             dateChanged);
    $scope.$watch("selectors.test.curVal",      testChanged);
    $scope.$watch("selectors.language.curVal",  languageChanged);
    $scope.$watch("selectors.client.curVal",    clientChanged);

	/* ---------- start the ball rolling by getting the json file -------------- */
	var jsonFile = './results/results.json';
	var lastDates = [];
    $http({ method: 'GET', url: jsonFile })
        .then(function successCallback(response) {
			results = response.data;

			// initialize the pinned charts list
			var pinned = angular.fromJson($localStorage['pinned2']);
			$scope.pinned = pinned ? pinned : [];

			if ($scope.tabState.saved)
				showSavedCharts();

	        var dates = [];
	        var years = Object.keys(results);
	        years.forEach (function (year) {
	            var months = Object.keys(results[year])
	            months.forEach (function (month) {
	                var days = Object.keys(results[year][month])
	                days.forEach (function (day) {
	                    var d = year + '-' + month + '-' + day + 'T23:59:59Z';
	                    dates.push (d);
	                    lastDates.push(new Date(d));
	                })
	            })
	        })

			dates.sort();
			lastDates.sort( function (a,b) { return a-b });

	        $scope.minDate = new Date(dates[0]);
	        $scope.maxDate = new Date(dates[dates.length - 1]);
	        $scope.selectors.date = $scope.maxDate; // triggers the date watcher
	    }, function errorCallback(response) {
			$scope.pinned = [];
			var msg;
			// if an exception was thrown, response will be the exception
			if (response.stack) {
				msg = "Unable to parse the json file: " + jsonFile + " " + response.message;
	        } else {
				msg = response.statusText + ': ' + jsonFile;
	        }
			$scope.alert = { type: 'danger', msg: msg };
        })

	var descriptionFile = './results/descriptions.json';
    $http({ method: 'GET', url: descriptionFile })
        .then(function successCallback(response) {
			descriptions = response.data;

	    }, function errorCallback(response) {
			var msg;
			// if an exception was thrown, response will be the exception
			if (response.stack) {
				msg = "Unable to parse the descriptions file: " + descriptionFile + " " + response.message;
	        } else {
				msg = response.statusText + ': ' + descriptionFile;
	        }
			console.log( msg );
        })
	$scope.showDescription = function () {
		if (descriptions && $scope.selectors.test.curVal && descriptions[$scope.selectors.test.curVal])
			return descriptions[$scope.selectors.test.curVal].description
	}

});
