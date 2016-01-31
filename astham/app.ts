/// <reference path="jquery/jqueryui.d.ts"/>
// add center() to JQuery
function jQueryCenter(el, parent) {
    el.css("position", "absolute");
    el.css("top", Math.max(0, (($(parent).height() - $(el).outerHeight()) / 2) +
        $(parent).scrollTop()) + "px");
    el.css("left", Math.max(0, (($(parent).width() - $(el).outerWidth()) / 2) +
        $(parent).scrollLeft()) + "px");
    return el;
}
function containsSameLetter(s1, s2) {
    if (s1.length != s2.length)
        return false;
    return s1.split('').sort().join('') == s2.split('').sort().join('');
}
;
var oActive, g_lastX, g_lastY, nZFocus = 100;
var g_fLeftButtonDown = false;
var g_fIsTracking = false;
/// the text insertion point
var InsertionPoint = (function () {
    function InsertionPoint() {
        this.goHome();
    }
    InsertionPoint.prototype.goHome = function () {
        this.y = 20;
        this.goStartOfLine();
    };
    InsertionPoint.prototype.goStartOfLine = function () {
        this.x = 10;
    };
    InsertionPoint.prototype.goLineDown = function () {
        this.y += 46;
    };
    InsertionPoint.prototype.moveRight = function () {
        this.x += 46;
    };
    InsertionPoint.prototype.moveTo = function (x, y) {
        this.x = x;
        this.y = y;
    };
    InsertionPoint.prototype.moveOnTopOf = function (e) {
        if ("offsetLeft" in e) {
            this.x = e.offsetLeft;
            this.y = e.offsetTop;
        }
        else if ("x" in e) {
            this.x = e.x;
            this.y = e.y;
        }
    };
    return InsertionPoint;
})();
var g_insertionPoint = new InsertionPoint();
var g_anchorPoint = g_insertionPoint;
/// keeps track of how many items been inserted, to get a unique id for each element
var creationCounter = 0;
/// the selected elements
var Tiles = (function () {
    function Tiles() {
        this.tiles = new Array();
    }
    Tiles.prototype.clear = function () {
        this.tiles.forEach(function (e) {
            e.setAttribute("class", "tile selectable");
        });
        this.tiles = [];
    };
    Tiles.prototype.push = function (e) {
        e.setAttribute("class", "tile selectable selected");
        this.tiles.push(e);
    };
    ;
    Tiles.prototype.indexOf = function (e) { return this.tiles.indexOf(e); };
    ;
    return Tiles;
})();
var g_selectedTiles = new Tiles();
function makeLetter(parent, letter) {
    var idCounter = String(creationCounter++);
    var el = document.createElementNS("http://www.w3.org/2000/svg", "svg");
    el.id = "svg" + idCounter;
    el.setAttribute("class", "tile selectable");
    el.x.baseVal.newValueSpecifiedUnits(SVGLength.SVG_LENGTHTYPE_PX, g_insertionPoint.x);
    el.y.baseVal.newValueSpecifiedUnits(SVGLength.SVG_LENGTHTYPE_PX, g_insertionPoint.y);
    el.width.baseVal.newValueSpecifiedUnits(SVGLength.SVG_LENGTHTYPE_PX, 40);
    el.height.baseVal.newValueSpecifiedUnits(SVGLength.SVG_LENGTHTYPE_PX, 40);
    g_insertionPoint.moveRight();
    g_anchorPoint = g_insertionPoint;
    // add rectangle
    var nsUri = "http://www.w3.org/2000/svg";
    var r = document.createElementNS(nsUri, "rect");
    r.id = "rect" + idCounter;
    r.className.baseVal = "tileBkg";
    r.setAttribute("height", "38");
    r.setAttribute("width", "38");
    r.setAttribute("rx", "4");
    r.setAttribute("ry", "4");
    el.appendChild(r);
    // add tile text
    var t = document.createElementNS(nsUri, "text");
    t.id = "text" + idCounter;
    t.setAttribute("class", "tileText");
    t.setAttribute("x", "6");
    t.setAttribute("y", "30");
    t.textContent = letter;
    el.appendChild(t);
    // add lock icon
    var i = document.createElementNS(nsUri, "image");
    i.id = "img" + idCounter;
    i.className.baseVal = "invisible";
    i.setAttribute("x", "28");
    i.setAttribute("y", "27");
    i.setAttribute("height", "9px");
    i.setAttribute("width", "7px");
    i.setAttributeNS('http://www.w3.org/1999/xlink', 'href', 'images/locked.png');
    el.appendChild(i);
    // note: JQuery selectable(), draggable() does not work for SVG
    el.onmousedown = function (oPssEvt1) {
        var me = this;
        var bExit = true;
        var oMsEvent1 = oPssEvt1 || window.event;
        for (var iNode = (oMsEvent1.target || oMsEvent1.srcElement); iNode; iNode = iNode.parentNode) {
            if (iNode === this) {
                bExit = false;
                oActive = iNode;
                break;
            }
        }
        if (bExit) {
            return;
        }
        g_fLeftButtonDown = true;
        g_lastX = oMsEvent1.clientX;
        g_lastY = oMsEvent1.clientY;
        oActive.style.zIndex = nZFocus++;
        oPssEvt1.preventDefault();
        // if we are already selected, do nothing
        // if ctrl is down, do not clear selection
        //      regardless of ctrl key, add us to selection
        if (g_selectedTiles.indexOf(this) < 0) {
            if (!oMsEvent1.ctrlKey)
                g_selectedTiles.clear();
            g_selectedTiles.push(this);
        }
        return false;
    };
    el.setAttribute('locked', '0');
    parent.appendChild(el);
    return el;
}
function getNonLockedTiles() {
    var rslt = new Tiles;
    $(".tile").each(function (idx, e) {
        if (e.getAttribute('locked') != '1') {
            rslt.tiles.push(e);
        }
    });
    return rslt;
}
function getSelectedNonLockedTiles() {
    var rslt = new Tiles;
    g_selectedTiles.tiles.forEach(function (e) {
        if (e.getAttribute('locked') != '1') {
            rslt.tiles.push(e);
        }
    });
    return rslt;
}
function hackAnimate($el, attrs, speed) {
    // duration in ms
    speed = speed || 200;
    var start = {}, timeout = 20, steps = Math.floor(speed / timeout), cycles = steps; // counter for cycles left
    // populate the object with the initial state
    $.each(attrs, function (k, v) {
        start[k] = $el.attr(k);
    });
    (function loop() {
        $.each(attrs, function (k, v) {
            var pst = (v - start[k]) / steps; // how much to add at each step
            $el.attr(k, function (i, old) {
                return +old + pst; // add value do the old one
            });
        });
        if (--cycles)
            setTimeout(loop, timeout);
        else
            $el.attr(attrs);
    })(); // start the loop
}
function outputAnagramAtIndex(rslt, available) {
    var minX = 999999;
    var minY = 999999;
    available.tiles.forEach(function (e) {
        minX = Math.min(minX, e.x.baseVal.value);
        minY = Math.min(minY, e.y.baseVal.value);
    });
    if (rslt == null || rslt.length == 0)
        return;
    var board = document.getElementById("board");
    var replStr = rslt.toUpperCase();
    var el = document.getElementById("parentSVG");
    g_insertionPoint.moveTo(minX, minY);
    for (var i = 0; i < replStr.length; i++) {
        if (replStr.charCodeAt(i) == 32)
            g_insertionPoint.moveRight();
        else {
            var c = replStr[i].toUpperCase();
            for (var j = 0; j < available.tiles.length; j++) {
                var svgElem = available.tiles[j];
                var textElem = (svgElem.getElementsByTagName("text").item(0));
                if (textElem.textContent == c) {
                    hackAnimate($("#" + svgElem.id), { x: g_insertionPoint.x, y: g_insertionPoint.y }, 200 // optional duration in ms, defaults to 400
                        );
                    g_insertionPoint.moveRight();
                    if (g_insertionPoint.x >= board.clientWidth - 46) {
                        g_insertionPoint.goLineDown();
                        g_insertionPoint.goStartOfLine();
                    }
                    available.tiles.splice(j, 1);
                    break;
                }
            }
        }
    }
    ;
}
function selectAllTiles() {
    $(".tile").each(function (idx, el) {
        g_selectedTiles.tiles.push(el);
    });
}
function removeSelectedTiles() {
    var rg = getSelectedNonLockedTiles();
    rg.tiles.forEach(function (e) {
        $(e).remove();
    });
    g_selectedTiles.clear();
}
function focusBoard() {
    var board = $("#board");
    if (board.is(":visible")) {
        board.focus();
    }
}
window.onload = function () {
    // Animate loader off screen
    if (!browserSupported()) {
        var divDlg = $("<div id = 'dialog' title='Update Your Browser'>Your browser does not support SVG graphics. Cannot continue!</div>");
        $("#board").append(divDlg);
        $("#dialog").dialog({
            dialogClass: "no-close",
            autoOpen: false,
            buttons: [
                {
                    text: "OK",
                    click: function () {
                        $(this).dialog("close");
                    }
                }
            ]
        });
        $("#dialog").dialog("open");
        return;
    }
    document.onmousemove = function (oPssEvt2) {
        if (!g_fLeftButtonDown) {
            return;
        }
        var oMsEvent2 = oPssEvt2 || window.event;
        oMsEvent2.preventDefault();
        var dx = oMsEvent2.clientX - g_lastX;
        var dy = oMsEvent2.clientY - g_lastY;
        g_selectedTiles.tiles.forEach(function (svg) {
            svg.setAttribute("x", String(svg.x.baseVal.value + dx));
            svg.setAttribute("y", String(svg.y.baseVal.value + dy));
        });
        g_lastX = oMsEvent2.clientX;
        g_lastY = oMsEvent2.clientY;
    };
    var board = document.getElementById("board");
    board.onmousedown = function (oPssEvt) {
        if (oPssEvt.defaultPrevented)
            return;
        // start tracking
        g_fIsTracking = true;
        // $("#big-ghost").remove();
        g_selectedTiles.clear();
        $(".ghost-select").addClass("ghost-active");
        $(".ghost-select").css({
            'left': oPssEvt.pageX,
            'top': oPssEvt.pageY
        });
        initialW = oPssEvt.pageX;
        initialH = oPssEvt.pageY;
        $(document).bind("mouseup", selectElements);
        $(document).bind("mousemove", openSelector);
    };
    document.onmousedown = function () {
        return false;
    };
    document.onmouseup = function () {
        g_fLeftButtonDown = false;
    };
    document.onkeydown = function (event) {
        var code = event.keyCode || event.charCode;
        if (code == 32) {
            g_insertionPoint.moveRight();
            event.preventDefault();
        }
        else if (code == 46) {
            // delete
            removeSelectedTiles();
            event.preventDefault();
        }
    };
    document.onkeypress = function (event) {
        if (event.defaultPrevented) {
            return; // Should do nothing if the key event was already consumed.
        }
        var code = event.keyCode || event.charCode;
        if (code == 13) {
            g_insertionPoint.goLineDown();
            g_insertionPoint.goStartOfLine();
        }
        else if (code == 9) {
            g_insertionPoint.moveRight();
        }
        else if (code == 27) {
            // escape
            selectAllTiles();
            removeSelectedTiles();
            g_insertionPoint.goHome();
        }
        else if (event.key || event.keyCode) {
            var k = event.key || String.fromCharCode(event.keyCode);
            if (k.length != 1) {
                g_insertionPoint.moveRight();
            }
            else {
                var el = document.getElementById("parentSVG");
                makeLetter(el, k.toUpperCase());
            }
        }
    };
    var initialString = "testing content";
    g_insertionPoint.y = (board.offsetHeight / 2) - 23;
    g_insertionPoint.x = (board.offsetWidth / 2) - (23 + initialString.length * 23);
    initialString.split('').forEach(function (s) {
        if (s == ' ') {
            g_insertionPoint.moveRight();
        }
        else {
            makeLetter(document.getElementById("parentSVG"), s.toUpperCase());
        }
    });
    // set up buttons and similar
    $("button#clear").button({
        icons: {
            primary: "ui-icon-trash"
        },
        text: false
    }).click(function (event) {
        event.preventDefault();
        g_selectedTiles.clear();
        $(".tile").remove();
        focusBoard();
        g_insertionPoint.goHome();
    });
    $("button#lock").button({
        icons: {
            primary: "ui-icon-locked"
        },
        text: false
    }).click(function (event) {
        event.preventDefault();
        g_selectedTiles.tiles.forEach(function (e) {
            var imgElem = e.getElementsByTagName("image").item(0);
            if (e.getAttribute('locked') == '0') {
                imgElem.setAttribute("class", "");
                e.setAttribute('locked', '1');
            }
            else {
                imgElem.setAttribute("class", "invisible");
                e.setAttribute('locked', '0');
            }
        });
        focusBoard();
    });
    $("button#lookup").button({
        icons: {
            primary: "ui-icon-transfer-e-w"
        },
        text: false
    }).click(function (event) {
        event.preventDefault();
        // get the selected letters
        var availableTiles = getNonLockedTiles();
        var language = $("#lang").val();
        var s = "";
        var x = null;
        availableTiles.tiles.forEach(function (e) {
            var textElem = (e.getElementsByTagName("text").item(0));
            s += textElem.textContent;
        });
        if (s.length == 0)
            return; // nuttin to do
        // move to next anagram, or previous if shift key is down
        var delta = 1;
        if (event.shiftKey) {
            delta = -1;
        }
        focusBoard();
        // check if we already have anagrams on this
        var idx = 0;
        if (Modernizr.localstorage) {
            // Yippee! We can use localStorage awesomeness
            var prevExpression = localStorage.getItem('strexpr');
            if (prevExpression && containsSameLetter(prevExpression, s) && localStorage.getItem('language') === language) {
                idx = Number(localStorage.getItem('rslt_idx')) || 0;
                var rslt = JSON.parse(localStorage.getItem('anagrams'));
                idx += delta;
                if (idx < 0) {
                    idx = rslt.length - 1;
                }
                if (idx >= rslt.length) {
                    idx = 0;
                }
                outputAnagramAtIndex(rslt[idx], availableTiles);
                localStorage['rslt_idx'] = idx;
                return; // no need to ask for more
            }
        }
        // call the astham api here
        var xhr = new XMLHttpRequest();
        var cmd = locationHREFWithoutResource() + "api/v1/a/" + encodeURIComponent(s) + "/" + language;
        xhr.open("GET", cmd, true);
        // change to spinning button icon
        $("#spinning-refresh").addClass("glyphicon-spin");
        xhr.send();
        // add the listeners
        var lookup = $("button#lookup");
        var clear = $("button#clear");
        function detach() {
            // remove spinning button icon
            $("#spinning-refresh").removeClass("glyphicon-spin");
            clear.off("click", cancelling);
            lookup.off("click", cancelling);
        }
        function cancelling() {
            detach();
            xhr.abort();
        }
        clear.on("click", cancelling);
        lookup.on("click", cancelling);
        xhr.onreadystatechange = function () {
            if (xhr.status != 200) {
                detach();
                if (xhr.status > 0)
                    alert("Server error " + xhr.status + "\n" + xhr.statusText);
                return;
            }
            if (xhr.readyState == 4) {
                var headers = xhr.getAllResponseHeaders();
                // skip any BOM characters at start of responseText
                var fixup = "";
                var first_character = 0;
                for (var i = first_character; i < xhr.responseText.length; i++) {
                    fixup += xhr.responseText[i];
                }
                var rslt = JSON.parse(fixup);
                detach();
                if (rslt.anagrams.length == 0) {
                    alert("Cannot make anything of that");
                    return;
                }
                // if single result, convert to array
                if (rslt.anagrams && rslt.anagrams.constructor === String) {
                    var tmp = [rslt.anagrams];
                    rslt.anagrams = tmp;
                }
                outputAnagramAtIndex(rslt.anagrams[idx], availableTiles);
                // store index and result in the local storage, if available
                if (Modernizr.localstorage) {
                    localStorage['rslt_idx'] = idx;
                    localStorage['strexpr'] = s;
                    localStorage['anagrams'] = JSON.stringify(rslt.anagrams);
                    localStorage['language'] = language;
                }
            }
        };
    });
};
function randomShuffleArray(array) {
    var currentIndex = array.length, temporaryValue, randomIndex;
    // While there remain elements to shuffle...
    while (0 !== currentIndex) {
        // Pick a remaining element...
        randomIndex = Math.floor(Math.random() * currentIndex);
        currentIndex -= 1;
        // And swap it with the current element.
        temporaryValue = array[currentIndex];
        array[currentIndex] = array[randomIndex];
        array[randomIndex] = temporaryValue;
    }
    return array;
}
var initialW = 0;
var initialH = 0;
function selectElements(e) {
    g_fIsTracking = false;
    $(document).unbind("mousemove", openSelector);
    $(document).unbind("mouseup", selectElements);
    var maxX = 0;
    var minX = 5000;
    var maxY = 0;
    var minY = 5000;
    $(".selectable").each(function () {
        var aElem = $(".ghost-select");
        var bElem = $(this);
        var aRect = document.getElementById("ghost-select").getBoundingClientRect();
        var bRect = this.getBoundingClientRect();
        var result = doRectsIntersect(aRect, bRect);
        if (result == true) {
            // the checking of min max coordinates is broken, from here ...
            var aElemPos = bElem.offset();
            var bElemPos = bElem.offset();
            var aW = bElem.width();
            var aH = bElem.height();
            var bW = bElem.width();
            var bH = bElem.height();
            var coords = checkMaxMinPos(aElemPos, bElemPos, aW, aH, bW, bH, maxX, minX, maxY, minY);
            maxX = coords.maxX;
            minX = coords.minX;
            maxY = coords.maxY;
            minY = coords.minY;
            // ... to here
            var parent = bElem.parent();
            g_selectedTiles.push(this);
            //console.log(aElem, bElem,maxX, minX, maxY,minY);
            if (bElem.css("left") === "auto" && bElem.css("top") === "auto") {
                bElem.css({
                    'left': parent.css('left'),
                    'top': parent.css('top')
                });
            }
        }
    });
    $(".ghost-select").removeClass("ghost-active");
    $(".ghost-select").width(0).height(0);
    ////////////////////////////////////////////////
}
function openSelector(e) {
    var w = Math.abs(initialW - e.pageX);
    var h = Math.abs(initialH - e.pageY);
    $(".ghost-select").css({
        'width': w,
        'height': h
    });
    if (e.pageX <= initialW && e.pageY >= initialH) {
        $(".ghost-select").css({
            'left': e.pageX
        });
    }
    else if (e.pageY <= initialH && e.pageX >= initialW) {
        $(".ghost-select").css({
            'top': e.pageY
        });
    }
    else if (e.pageY < initialH && e.pageX < initialW) {
        $(".ghost-select").css({
            'left': e.pageX,
            "top": e.pageY
        });
    }
}
function doRectsIntersect(r1, r2) {
    return !(r2.left > r1.right
        || r2.right < r1.left
        || r2.top > r1.bottom
        || r2.bottom < r1.top);
}
function checkMaxMinPos(a, b, aW, aH, bW, bH, maxX, minX, maxY, minY) {
    'use strict';
    if (a.left < b.left) {
        if (a.left < minX) {
            minX = a.left;
        }
    }
    else {
        if (b.left < minX) {
            minX = b.left;
        }
    }
    if (a.left + aW > b.left + bW) {
        if (a.left > maxX) {
            maxX = a.left + aW;
        }
    }
    else {
        if (b.left + bW > maxX) {
            maxX = b.left + bW;
        }
    }
    ////////////////////////////////
    if (a.top < b.top) {
        if (a.top < minY) {
            minY = a.top;
        }
    }
    else {
        if (b.top < minY) {
            minY = b.top;
        }
    }
    if (a.top + aH > b.top + bH) {
        if (a.top > maxY) {
            maxY = a.top + aH;
        }
    }
    else {
        if (b.top + bH > maxY) {
            maxY = b.top + bH;
        }
    }
    return {
        'maxX': maxX,
        'minX': minX,
        'maxY': maxY,
        'minY': minY
    };
}
function locationHREFWithoutResource() {
    var pathWORes = location.pathname.substring(0, location.pathname.lastIndexOf("/") + 1);
    var protoWDom = location.href.substr(0, location.href.indexOf("/", 8));
    return protoWDom + pathWORes;
};