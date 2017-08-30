/*******************************************************************************
 * Copyright (c) 2017 Nicola Del Gobbo
 * Licensed under the Apache License, Version 2.0 (the "License"); you may not
 * use this file except in compliance with the License. You may obtain a copy of
 * the license at http://www.apache.org/licenses/LICENSE-2.0
 *
 * THIS CODE IS PROVIDED ON AN *AS IS* BASIS, WITHOUT WARRANTIES OR CONDITIONS
 * OF ANY KIND, EITHER EXPRESS OR IMPLIED, INCLUDING WITHOUT LIMITATION ANY
 * IMPLIED WARRANTIES OR CONDITIONS OF TITLE, FITNESS FOR A PARTICULAR PURPOSE,
 * MERCHANTABLITY OR NON-INFRINGEMENT.
 *
 * See the Apache Version 2.0 License for specific language governing
 * permissions and limitations under the License.
 *
 * Contributors - initial API implementation:
 * Nicola Del Gobbo <nicoladelgobbo@gmail.com>
 * Mauro Doganieri <mauro.doganieri@gmail.com>
 ******************************************************************************/

#include <nan.h>
#include "buildinfo.h"

//////////////////////////// INIT & CONFIG MODULE //////////////////////////////

NAN_MODULE_INIT(Init)
{

    Nan::Set(target, Nan::New("VERSION").ToLocalChecked(), Nan::New(Heif::VERSION).ToLocalChecked());  
    Nan::Set(target, Nan::New("MAJOR").ToLocalChecked(), Nan::New(Heif::MAJOR)); 
    Nan::Set(target, Nan::New("MINOR").ToLocalChecked(), Nan::New(Heif::MINOR)); 
    Nan::Set(target, Nan::New("PATCH").ToLocalChecked(), Nan::New(Heif::PATCH));
    Nan::Set(target, Nan::New("CODE_NAME").ToLocalChecked(), Nan::New(Heif::CODE_NAME).ToLocalChecked());             
}

NODE_MODULE(Heif, Init)

////////////////////////////////////////////////////////////////////////////////