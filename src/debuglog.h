//@ SPDX-FileCopyrightText: 2024-present roundedrectangle
//@ SPDX-FileCopyrightText: 2020 Slava Monich et al
//@ SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <QLoggingCategory>

#ifndef DEBUG_ROOT_MODULE
#define DEBUG_ROOT_MODULE "yaqtlib"
#endif

#ifndef DEBUG_MODULE
#define DEBUG_MODULE Debug
#endif

#define LOG_CATEGORY__(x) x##Log
#define LOG_CATEGORY_(x) LOG_CATEGORY__(x)
#define LOG_CATEGORY LOG_CATEGORY_(DEBUG_MODULE)
static const QLoggingCategory LOG_CATEGORY(DEBUG_ROOT_MODULE "." QT_STRINGIFY(DEBUG_MODULE));
#define LOG(x) qCDebug(LOG_CATEGORY) << "[" QT_STRINGIFY(DEBUG_MODULE) "]" << x
#define WARN(x) qCWarning(LOG_CATEGORY) << "[" QT_STRINGIFY(DEBUG_MODULE) "]" << x

// No VERBOSE in release build
#ifndef VERBOSE
#  if defined (QT_DEBUG) || defined (DEBUG)
#    define VERBOSE(x) LOG(x)
#  else
#    define VERBOSE(x)
#  endif
#endif
