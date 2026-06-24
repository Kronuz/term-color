/*
 * Copyright (c) 2015-2019 Dubalu LLC
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */

#pragma once

#include "ansi_color.hh"      // for ansi_color

static constexpr auto NO_COLOR = no_color();
static constexpr auto CLEAR_COLOR = clear_color();

static constexpr auto ALICE_BLUE = rgb(240, 248, 255);

static constexpr auto ANTIQUE_WHITE = rgb(250, 235, 215);

static constexpr auto AQUA = rgb(0, 255, 255);

static constexpr auto AQUA_MARINE = rgb(127, 255, 212);

static constexpr auto AZURE = rgb(240, 255, 255);

static constexpr auto BEIGE = rgb(245, 245, 220);

static constexpr auto BISQUE = rgb(255, 228, 196);

static constexpr auto BLACK = rgb(0, 0, 0);

static constexpr auto BLANCHED_ALMOND = rgb(255, 235, 205);

static constexpr auto BLUE = rgb(0, 0, 255);

static constexpr auto BLUE_VIOLET = rgb(138, 43, 226);

static constexpr auto BROWN = rgb(165, 42, 42);

static constexpr auto BURLY_WOOD = rgb(222, 184, 135);

static constexpr auto CADET_BLUE = rgb(95, 158, 160);

static constexpr auto CHARTREUSE = rgb(127, 255, 0);

static constexpr auto CHOCOLATE = rgb(210, 105, 30);

static constexpr auto CORAL = rgb(255, 127, 80);

static constexpr auto CORNFLOWER_BLUE = rgb(100, 149, 237);

static constexpr auto CORNSILK = rgb(255, 248, 220);

static constexpr auto CRIMSON = rgb(220, 20, 60);

static constexpr auto CYAN = rgb(0, 255, 255);

static constexpr auto DARK_BLUE = rgb(0, 0, 139);

static constexpr auto DARK_CORAL = rgb(205, 91, 69);

static constexpr auto DARK_CYAN = rgb(0, 139, 139);

static constexpr auto DARK_GOLDEN_ROD = rgb(184, 134, 11);

static constexpr auto DARK_GRAY = rgb(169, 169, 169);

static constexpr auto DARK_GREEN = rgb(0, 100, 0);

static constexpr auto DARK_GREY = rgb(169, 169, 169);

static constexpr auto DARK_KHAKI = rgb(189, 183, 107);

static constexpr auto DARK_MAGENTA = rgb(139, 0, 139);

static constexpr auto DARK_OLIVE_GREEN = rgb(85, 107, 47);

static constexpr auto DARK_ORANGE = rgb(255, 140, 0);

static constexpr auto DARK_ORCHID = rgb(153, 50, 204);

static constexpr auto DARK_RED = rgb(139, 0, 0);

static constexpr auto DARK_SALMON = rgb(233, 150, 122);

static constexpr auto DARK_SEA_GREEN = rgb(143, 188, 143);

static constexpr auto DARK_SLATE_BLUE = rgb(72, 61, 139);

static constexpr auto DARK_SLATE_GRAY = rgb(47, 79, 79);

static constexpr auto DARK_SLATE_GREY = rgb(47, 79, 79);

static constexpr auto DARK_STEEL_BLUE = rgb(49, 90, 125);

static constexpr auto DARK_TURQUOISE = rgb(0, 206, 209);

static constexpr auto DARK_VIOLET = rgb(148, 0, 211);

static constexpr auto DEEP_PINK = rgb(255, 20, 147);

static constexpr auto DEEP_SKY_BLUE = rgb(0, 191, 255);

static constexpr auto DIM_GRAY = rgb(105, 105, 105);

static constexpr auto DIM_GREY = rgb(105, 105, 105);

static constexpr auto DODGER_BLUE = rgb(30, 144, 255);

static constexpr auto FIRE_BRICK = rgb(178, 34, 34);

static constexpr auto FLORAL_WHITE = rgb(255, 250, 240);

static constexpr auto FOREST_GREEN = rgb(34, 139, 34);

static constexpr auto FUCHSIA = rgb(255, 0, 255);

static constexpr auto GAINSBORO = rgb(220, 220, 220);

static constexpr auto GHOST_WHITE = rgb(248, 248, 255);

static constexpr auto GOLD = rgb(255, 215, 0);

static constexpr auto GOLDEN_ROD = rgb(218, 165, 32);

static constexpr auto GRAY = rgb(128, 128, 128);

static constexpr auto GREEN = rgb(0, 128, 0);

static constexpr auto GREEN_YELLOW = rgb(173, 255, 47);

static constexpr auto GREY = rgb(128, 128, 128);

static constexpr auto HONEY_DEW = rgb(240, 255, 240);

static constexpr auto HOT_PINK = rgb(255, 105, 180);

static constexpr auto INDIAN_RED = rgb(205, 92, 92);

static constexpr auto INDIGO = rgb(75, 0, 130);

static constexpr auto IVORY = rgb(255, 255, 240);

static constexpr auto KHAKI = rgb(240, 230, 140);

static constexpr auto LAVENDER = rgb(230, 230, 250);

static constexpr auto LAVENDER_BLUSH = rgb(255, 240, 245);

static constexpr auto LAWN_GREEN = rgb(124, 252, 0);

static constexpr auto LEMON_CHIFFON = rgb(255, 250, 205);

static constexpr auto LIGHT_BLUE = rgb(173, 216, 230);

static constexpr auto LIGHT_CORAL = rgb(240, 128, 128);

static constexpr auto LIGHT_CYAN = rgb(224, 255, 255);

static constexpr auto LIGHT_GOLDEN_ROD_YELLOW = rgb(250, 250, 210);

static constexpr auto LIGHT_GRAY = rgb(211, 211, 211);

static constexpr auto LIGHT_GREEN = rgb(144, 238, 144);

static constexpr auto LIGHT_GREY = rgb(211, 211, 211);

static constexpr auto LIGHT_PINK = rgb(255, 182, 193);

static constexpr auto LIGHT_PURPLE = rgb(232, 10, 180);

static constexpr auto LIGHT_RED = rgb(232, 25, 10);

static constexpr auto LIGHT_SALMON = rgb(255, 160, 122);

static constexpr auto LIGHT_SEA_GREEN = rgb(32, 178, 170);

static constexpr auto LIGHT_SKY_BLUE = rgb(135, 206, 250);

static constexpr auto LIGHT_SLATE_GRAY = rgb(119, 136, 153);

static constexpr auto LIGHT_SLATE_GREY = rgb(119, 136, 153);

static constexpr auto LIGHT_STEEL_BLUE = rgb(176, 196, 222);

static constexpr auto LIGHT_YELLOW = rgb(255, 255, 224);

static constexpr auto LIME = rgb(0, 255, 0);

static constexpr auto LIME_GREEN = rgb(50, 205, 50);

static constexpr auto LINEN = rgb(250, 240, 230);

static constexpr auto MAGENTA = rgb(255, 0, 255);

static constexpr auto MAROON = rgb(128, 0, 0);

static constexpr auto MEDIUM_AQUA_MARINE = rgb(102, 205, 170);

static constexpr auto MEDIUM_BLUE = rgb(0, 0, 205);

static constexpr auto MEDIUM_ORCHID = rgb(186, 85, 211);

static constexpr auto MEDIUM_PURPLE = rgb(147, 112, 216);

static constexpr auto MEDIUM_SEA_GREEN = rgb(60, 179, 113);

static constexpr auto MEDIUM_SLATE_BLUE = rgb(123, 104, 238);

static constexpr auto MEDIUM_SPRING_GREEN = rgb(0, 250, 154);

static constexpr auto MEDIUM_TURQUOISE = rgb(72, 209, 204);

static constexpr auto MEDIUM_VIOLET_RED = rgb(199, 21, 133);

static constexpr auto MIDNIGHT_BLUE = rgb(25, 25, 112);

static constexpr auto MINT_CREAM = rgb(245, 255, 250);

static constexpr auto MISTY_ROSE = rgb(255, 228, 225);

static constexpr auto MOCCASIN = rgb(255, 228, 181);

static constexpr auto NAVAJO_WHITE = rgb(255, 222, 173);

static constexpr auto NAVY = rgb(0, 0, 128);

static constexpr auto OLD_LACE = rgb(253, 245, 230);

static constexpr auto OLIVE = rgb(128, 128, 0);

static constexpr auto OLIVE_DRAB = rgb(107, 142, 35);

static constexpr auto ORANGE = rgb(255, 165, 0);

static constexpr auto ORANGE_RED = rgb(255, 69, 0);

static constexpr auto ORCHID = rgb(218, 112, 214);

static constexpr auto PALE_GOLDEN_ROD = rgb(238, 232, 170);

static constexpr auto PALE_GREEN = rgb(152, 251, 152);

static constexpr auto PALE_TURQUOISE = rgb(175, 238, 238);

static constexpr auto PALE_VIOLET_RED = rgb(216, 112, 147);

static constexpr auto PAPAYA_WHIP = rgb(255, 239, 213);

static constexpr auto PEACH_PUFF = rgb(255, 218, 185);

static constexpr auto PERU = rgb(205, 133, 63);

static constexpr auto PINK = rgb(255, 192, 203);

static constexpr auto PLUM = rgb(221, 160, 221);

static constexpr auto POWDER_BLUE = rgb(176, 224, 230);

static constexpr auto PURPLE = rgb(128, 0, 128);

static constexpr auto RED = rgb(255, 0, 0);

static constexpr auto ROSY_BROWN = rgb(188, 143, 143);

static constexpr auto ROYAL_BLUE = rgb(65, 105, 225);

static constexpr auto SADDLE_BROWN = rgb(139, 69, 19);

static constexpr auto SALMON = rgb(250, 128, 114);

static constexpr auto SANDY_BROWN = rgb(244, 164, 96);

static constexpr auto SEA_GREEN = rgb(46, 139, 87);

static constexpr auto SEA_SHELL = rgb(255, 245, 238);

static constexpr auto SIENNA = rgb(160, 82, 45);

static constexpr auto SILVER = rgb(192, 192, 192);

static constexpr auto SKY_BLUE = rgb(135, 206, 235);

static constexpr auto SLATE_BLUE = rgb(106, 90, 205);

static constexpr auto SLATE_GRAY = rgb(112, 128, 144);

static constexpr auto SLATE_GREY = rgb(112, 128, 144);

static constexpr auto SNOW = rgb(255, 250, 250);

static constexpr auto SPRING_GREEN = rgb(0, 255, 127);

static constexpr auto STEEL_BLUE = rgb(70, 130, 180);

static constexpr auto TAN = rgb(210, 180, 140);

static constexpr auto TEAL = rgb(0, 128, 128);

static constexpr auto THISTLE = rgb(216, 191, 216);

static constexpr auto TOMATO = rgb(255, 99, 71);

static constexpr auto TURQUOISE = rgb(64, 224, 208);

static constexpr auto VIOLET = rgb(238, 130, 238);

static constexpr auto WHEAT = rgb(245, 222, 179);

static constexpr auto WHITE = rgb(255, 255, 255);

static constexpr auto WHITE_SMOKE = rgb(245, 245, 245);

static constexpr auto YELLOW = rgb(255, 255, 0);

static constexpr auto YELLOW_GREEN = rgb(154, 205, 50);

