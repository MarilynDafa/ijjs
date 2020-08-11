var offsets = {"Etc/GMT+12":720,"Pacific/Pago_Pago":660,"Pacific/Midway":660,"Pacific/Honolulu":600,"America/Juneau":540,"America/Los_Angeles":480,"America/Tijuana":480,"America/Phoenix":420,"America/Chihuahua":420,"America/Mazatlan":420,"America/Denver":420,"America/Guatemala":360,"America/Chicago":360,"America/Mexico_City":360,"America/Monterrey":360,"America/Regina":360,"America/Bogota":300,"America/New_York":300,"America/Indiana/Indianapolis":300,"America/Lima":300,"America/Halifax":240,"America/Caracas":240,"America/Guyana":240,"America/La_Paz":240,"America/Puerto_Rico":240,"America/Santiago":240,"America/St_Johns":210,"America/Sao_Paulo":180,"America/Argentina/Buenos_Aires":180,"America/Godthab":180,"America/Montevideo":180,"Atlantic/South_Georgia":120,"Atlantic/Azores":60,"Atlantic/Cape_Verde":60,"Africa/Casablanca":0,"Europe/London":0,"Europe/Lisbon":0,"Africa/Monrovia":0,"Etc/UTC":0,"Europe/Amsterdam":-60,"Europe/Belgrade":-60,"Europe/Berlin":-60,"Europe/Zurich":-60,"Europe/Bratislava":-60,"Europe/Brussels":-60,"Europe/Budapest":-60,"Europe/Copenhagen":-60,"Europe/Dublin":-60,"Europe/Ljubljana":-60,"Europe/Madrid":-60,"Europe/Paris":-60,"Europe/Prague":-60,"Europe/Rome":-60,"Europe/Sarajevo":-60,"Europe/Skopje":-60,"Europe/Stockholm":-60,"Europe/Vienna":-60,"Europe/Warsaw":-60,"Africa/Algiers":-60,"Europe/Zagreb":-60,"Europe/Athens":-120,"Europe/Bucharest":-120,"Africa/Cairo":-120,"Africa/Harare":-120,"Europe/Helsinki":-120,"Asia/Jerusalem":-120,"Europe/Kaliningrad":-120,"Europe/Kiev":-120,"Africa/Johannesburg":-120,"Europe/Riga":-120,"Europe/Sofia":-120,"Europe/Tallinn":-120,"Europe/Vilnius":-120,"Asia/Baghdad":-180,"Europe/Istanbul":-180,"Asia/Kuwait":-180,"Europe/Minsk":-180,"Europe/Moscow":-180,"Africa/Nairobi":-180,"Asia/Riyadh":-180,"Europe/Volgograd":-180,"Asia/Tehran":-210,"Asia/Muscat":-240,"Asia/Baku":-240,"Europe/Samara":-240,"Asia/Tbilisi":-240,"Asia/Yerevan":-240,"Asia/Kabul":-270,"Asia/Yekaterinburg":-300,"Asia/Karachi":-300,"Asia/Tashkent":-300,"Asia/Kolkata":-330,"Asia/Colombo":-330,"Asia/Kathmandu":-345,"Asia/Almaty":-360,"Asia/Dhaka":-360,"Asia/Urumqi":-360,"Asia/Rangoon":-390,"Asia/Bangkok":-420,"Asia/Jakarta":-420,"Asia/Krasnoyarsk":-420,"Asia/Novosibirsk":-420,"Asia/Shanghai":-480,"Asia/Chongqing":-480,"Asia/Hong_Kong":-480,"Asia/Irkutsk":-480,"Asia/Kuala_Lumpur":-480,"Australia/Perth":-480,"Asia/Singapore":-480,"Asia/Taipei":-480,"Asia/Ulaanbaatar":-480,"Asia/Tokyo":-540,"Asia/Seoul":-540,"Asia/Yakutsk":-540,"Australia/Adelaide":-570,"Australia/Darwin":-570,"Australia/Brisbane":-600,"Australia/Melbourne":-600,"Pacific/Guam":-600,"Australia/Hobart":-600,"Pacific/Port_Moresby":-600,"Australia/Sydney":-600,"Asia/Vladivostok":-600,"Asia/Magadan":-660,"Pacific/Noumea":-660,"Pacific/Guadalcanal":-660,"Asia/Srednekolymsk":-660,"Pacific/Auckland":-720,"Pacific/Fiji":-720,"Asia/Kamchatka":-720,"Pacific/Majuro":-720,"Pacific/Chatham":-765,"Pacific/Tongatapu":-780,"Pacific/Apia":-780,"Pacific/Fakaofo":-780}

function offsetOf(timezone){
    var offset = offsets[timezone];
    if(offset != undefined && offset != null){
        return offset;
    } else {
        throw Error("Invalid timezone "+ timezone);
    }
}

function removeOffset(date){
    var currentOffset = date.getTimezoneOffset() * -60000;
    return date.getTime() - currentOffset;
}

function tz_timeAt(date, timezone){
    let timeUtc = removeOffset(date);
    var offset = offsetOf(timezone) * -60000;
    return new Date(timeUtc + offset);
}

//!month-names-conversion.js
var months = ['january', 'february', 'march', 'april', 'may', 'june', 'july',
    'august', 'september', 'october', 'november', 'december'];
var shortMonths = ['jan', 'feb', 'mar', 'apr', 'may', 'jun', 'jul', 'aug',
    'sep', 'oct', 'nov', 'dec'];

function convertMonthName(expression, items) {
    for (var i = 0; i < items.length; i++) {
        expression = expression.replace(new RegExp(items[i], 'gi'), parseInt(i, 10) + 1);
    }
    return expression;
}

function monthNamesConversion(monthExpression) {
    monthExpression = convertMonthName(monthExpression, months);
    monthExpression = convertMonthName(monthExpression, shortMonths);
    return monthExpression;
}

//!week-day-names-conversion.js

var weekDays = ['sunday', 'monday', 'tuesday', 'wednesday', 'thursday',
    'friday', 'saturday'];
var shortWeekDays = ['sun', 'mon', 'tue', 'wed', 'thu', 'fri', 'sat'];

function convertWeekDayName(expression, items) {
    for (var i = 0; i < items.length; i++) {
        expression = expression.replace(new RegExp(items[i], 'gi'), parseInt(i, 10));
    }
    return expression;
}

function weekDayNamesConversion(expression) {
    expression = expression.replace('7', '0');
    expression = convertWeekDayName(expression, weekDays);
    return convertWeekDayName(expression, shortWeekDays);
}

//!asterisk-to-range-conversion.js

function convertAsterisk(expression, replecement) {
    if (expression.indexOf('*') !== -1) {
        return expression.replace('*', replecement);
    }
    return expression;
}

function convertAsterisksToRanges(expressions) {
    expressions[0] = convertAsterisk(expressions[0], '0-59');
    expressions[1] = convertAsterisk(expressions[1], '0-59');
    expressions[2] = convertAsterisk(expressions[2], '0-23');
    expressions[3] = convertAsterisk(expressions[3], '1-31');
    expressions[4] = convertAsterisk(expressions[4], '1-12');
    expressions[5] = convertAsterisk(expressions[5], '0-6');
    return expressions;
}

//!range-conversion.js

function replaceWithRange(expression, text, init, end) {

    var numbers = [];
    var last = parseInt(end);
    var first = parseInt(init);

    if (first > last) {
        last = parseInt(init);
        first = parseInt(end);
    }

    for (var i = first; i <= last; i++) {
        numbers.push(i);
    }

    return expression.replace(new RegExp(text, 'gi'), numbers.join());
}

function _convertRange(expression) {
    var rangeRegEx = /(\d+)\-(\d+)/;
    var match = rangeRegEx.exec(expression);
    while (match !== null && match.length > 0) {
        expression = replaceWithRange(expression, match[0], match[1], match[2]);
        match = rangeRegEx.exec(expression);
    }
    return expression;
}

function convertRanges(expressions) {
    for (var i = 0; i < expressions.length; i++) {
        expressions[i] = _convertRange(expressions[i]);
    }
    return expressions;
}


//!step-values-conversion.js


function convertSteps(expressions) {
    var stepValuePattern = /^(.+)\/(\d+)$/;
    for (var i = 0; i < expressions.length; i++) {
        var match = stepValuePattern.exec(expressions[i]);
        var isStepValue = match !== null && match.length > 0;
        if (isStepValue) {
            var values = match[1].split(',');
            var setpValues = [];
            var divider = parseInt(match[2], 10);
            for (var j = 0; j <= values.length; j++) {
                var value = parseInt(values[j], 10);
                if (value % divider === 0) {
                    setpValues.push(value);
                }
            }
            expressions[i] = setpValues.join(',');
        }
    }
    return expressions;
}


function appendSeccondExpression(expressions) {
    if (expressions.length === 5) {
        return ['0'].concat(expressions);
    }
    return expressions;
}

function removeSpaces(str) {
    return str.replace(/\s{2,}/g, ' ').trim();
}

// Function that takes care of normalization.
function normalizeIntegers(expressions) {
    for (var i = 0; i < expressions.length; i++) {
        var numbers = expressions[i].split(',');
        for (var j = 0; j < numbers.length; j++) {
            numbers[j] = parseInt(numbers[j]);
        }
        expressions[i] = numbers;
    }
    return expressions;
}

//!index.js
/*
 * The node-cron core allows only numbers (including multiple numbers e.g 1,2).
 * This module is going to translate the month names, week day names and ranges
 * to integers relatives.
 *
 * Month names example:
 *  - expression 0 1 1 January,Sep *
 *  - Will be translated to 0 1 1 1,9 *
 *
 * Week day names example:
 *  - expression 0 1 1 2 Monday,Sat
 *  - Will be translated to 0 1 1 1,5 *
 *
 * Ranges example:
 *  - expression 1-5 * * * *
 *  - Will be translated to 1,2,3,4,5 * * * *
 */
function convertExpression(expression) {
    var expressions = removeSpaces(expression).split(' ');
    expressions = appendSeccondExpression(expressions);
    expressions[4] = monthNamesConversion(expressions[4]);
    expressions[5] = weekDayNamesConversion(expressions[5]);
    expressions = convertAsterisksToRanges(expressions);
    expressions = convertRanges(expressions);
    expressions = convertSteps(expressions);

    expressions = normalizeIntegers(expressions);

    return expressions.join(' ');
}

//!pattern-validation.js

function isValidExpression(expression, min, max) {
    var options = expression.split(',');
    var regexValidation = /^\d+$|^\*$|^\*\/\d+$/;
    for (var i = 0; i < options.length; i++) {
        var option = options[i];
        var optionAsInt = parseInt(options[i], 10);
        if (optionAsInt < min || optionAsInt > max || !regexValidation.test(option)) {
            return false;
        }
    }
    return true;
}

function isInvalidSecond(expression) {
    return !isValidExpression(expression, 0, 59);
}

function isInvalidMinute(expression) {
    return !isValidExpression(expression, 0, 59);
}

function isInvalidHour(expression) {
    return !isValidExpression(expression, 0, 23);
}

function isInvalidDayOfMonth(expression) {
    return !isValidExpression(expression, 1, 31);
}

function isInvalidMonth(expression) {
    return !isValidExpression(expression, 1, 12);
}

function isInvalidWeekDay(expression) {
    return !isValidExpression(expression, 0, 7);
}

function validateFields(patterns, executablePatterns) {
    if (isInvalidSecond(executablePatterns[0])) {
        throw patterns[0] + ' is a invalid expression for second';
    }

    if (isInvalidMinute(executablePatterns[1])) {
        throw patterns[1] + ' is a invalid expression for minute';
    }

    if (isInvalidHour(executablePatterns[2])) {
        throw patterns[2] + ' is a invalid expression for hour';
    }

    if (isInvalidDayOfMonth(executablePatterns[3])) {
        throw patterns[3] + ' is a invalid expression for day of month';
    }

    if (isInvalidMonth(executablePatterns[4])) {
        throw patterns[4] + ' is a invalid expression for month';
    }

    if (isInvalidWeekDay(executablePatterns[5])) {
        throw patterns[5] + ' is a invalid expression for week day';
    }
}

function validatePattern(pattern) {
    if (typeof pattern !== 'string') {
        throw 'pattern must be a string!';
    }

    var patterns = pattern.split(' ');
    var executablePattern = convertExpression(pattern);
    var executablePatterns = executablePattern.split(' ');

    if (patterns.length === 5) {
        patterns = ['0'].concat(patterns);
    }

    validateFields(patterns, executablePatterns);
}


//!task.js
function matchPattern(pattern, value) {
    if (pattern.indexOf(',') !== -1) {
        var patterns = pattern.split(',');
        return patterns.indexOf(value.toString()) !== -1;
    }
    return pattern === value.toString();
}

function mustRun(task, date) {
    var runInSecond = matchPattern(task.expressions[0], date.getSeconds());
    var runOnMinute = matchPattern(task.expressions[1], date.getMinutes());
    var runOnHour = matchPattern(task.expressions[2], date.getHours());
    var runOnDayOfMonth = matchPattern(task.expressions[3], date.getDate());
    var runOnMonth = matchPattern(task.expressions[4], date.getMonth() + 1);
    var runOnDayOfWeek = matchPattern(task.expressions[5], date.getDay());

    var runOnDay = false;
    var delta = task.initialPattern.length === 6 ? 0 : -1;

    if (task.initialPattern[3 + delta] === '*') {
        runOnDay = runOnDayOfWeek;
    } else if (task.initialPattern[5 + delta] === '*') {
        runOnDay = runOnDayOfMonth;
    } else {
        runOnDay = runOnDayOfMonth || runOnDayOfWeek;
    }

    return runInSecond && runOnMinute && runOnHour && runOnDay && runOnMonth;
}

function Task(pattern, execution) {
    validatePattern(pattern);
    this.initialPattern = pattern.split(' ');
    this.pattern = convertExpression(pattern);
    this.execution = execution;
    this.expressions = this.pattern.split(' ');

    //events.EventEmitter.call(this);

    this.update = (date) => {
        if (mustRun(this, date)) {
            new Promise((resolve, reject) => {
                //this.status = 'started';
                var ex = this.execution();
                if (ex instanceof Promise) {
                    ex.then(resolve).catch(reject);
                } else {
                    resolve();
                }
            }).then(() => {
                //this.status = 'done';
            }).catch((error) => {
                console.error(error);
                //this.status = 'failed';
            });
        }
    };
}


//!scheduled-task.js

/**
* Creates a new scheduled task.
*
* @param {Task} task - task to schedule.
* @param {*} options - task options.
*/
function ScheduledTask(task, options) {
    var timezone = options.timezone;

    /**
    * Starts updating the task.
    *
    * @returns {ScheduledTask} instance of this task.
    */
    this.start = () => {
        this.status = 'scheduled';
        if (this.task && !this.tick) {
            this.tick = setTimeout(this.task, 1000 - new Date().getMilliseconds() + 1);
        }

        return this;
    };

    /**
    * Stops updating the task.
    *
    * @returns {ScheduledTask} instance of this task.
    */
    this.stop = () => {
        this.status = 'stoped';
        if (this.tick) {
            clearTimeout(this.tick);
            this.tick = null;
        }

        return this;
    };

    /**
    * Returns the current task status.
    *
    * @returns {string} current task status.
    * The return may be:
    * - scheduled: when a task is scheduled and waiting to be executed.
    * - running: the task status while the task is executing. 
    * - stoped: when the task is stoped.
    * - destroyed: whe the task is destroyed, in that status the task cannot be re-started.
    * - failed: a task is maker as failed when the previous execution fails.
    */
    this.getStatus = () => {
        return this.status;
    };

    /**
    * Destroys the scheduled task.
    */
    this.destroy = () => {
        this.stop();
        this.status = 'destroyed';

        this.task = null;
    };
/*
    task.on('started', () => {
        this.status = 'running';
    });

    task.on('done', () => {
        this.status = 'scheduled';
    });

    task.on('failed', () => {
        this.status = 'failed';
    });
*/
    this.task = () => {
        var date = new Date();
        if (timezone) {
            date = tz_timeAt(date, timezone);
        }
        this.tick = setTimeout(this.task, 1000 - date.getMilliseconds() + 1);
        task.update(date);
    };

    this.tick = null;

    if (options.scheduled !== false) {
        this.start();
    }
}


//!cron.js
/**
 * Creates a new task to execute given function when the cron
 *  expression ticks.
 *
 * @param {string} expression - cron expression.
 * @param {Function} func - task to be executed.
 * @param {Object} options - a set of options for the scheduled task:
 *    - scheduled <boolean>: if a schaduled task is ready and running to be 
 *      performed when the time mach with the cron excpression.
 *    - timezone <string>: the tiemzone to execute the tasks.
 * 
 *    Example: 
 *    {
 *      "scheduled": true,
 *      "timezone": "America/Sao_Paulo"
 *    } 
 * 
 * @returns {ScheduledTask} update function.
 */
export function schedule(expression, func, options) {
    // Added for immediateStart depreciation
    if (typeof options === 'boolean') {
        console.warn('DEPRECIATION: imediateStart is deprecated and will be removed soon in favor of the options param.');
        options = {
            scheduled: options
        };
    }

    if (!options) {
        options = {
            scheduled: true
        };
    }

    var task = new Task(expression, func);
    return new ScheduledTask(task, options);
}

export function validate(expression) {
    try {
        validation(expression);
    } catch (e) {
        return false;
    }
    return true;
}