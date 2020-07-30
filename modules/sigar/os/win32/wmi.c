/*
 * Copyright (c) 2009 SpringSource, Inc.
 * Copyright (c) 2009 VMware, Inc.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#define UNICODE
#define _UNICODE

#include "sigar.h"



int sigar_parse_proc_args(sigar_t *sigar, wchar_t *buf,
                                     sigar_proc_args_t *procargs);

int sigar_proc_args_wmi_get(sigar_t *sigar, sigar_pid_t pid,
                                       sigar_proc_args_t *procargs)
{
    return 0;
}

int sigar_proc_exe_wmi_get(sigar_t *sigar, sigar_pid_t pid,
                                      sigar_proc_exe_t *procexe)
{
    return 0;
}
