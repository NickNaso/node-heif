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

const Heif =  require('../')

describe('Test heif', function () {
    
        it('Should return the version of Heif', function () {
            expect(Heif.VERSION).toBeDefined()
            expect(Heif.VERSION).not.toBe(null)
            expect(Heif.VERSION).toBe("2.0.0")
        })

        it('Should return the major version of Heif', function () {
            expect(Heif.MAJOR).toBeDefined()
            expect(Heif.MAJOR).not.toBe(null)
            expect(Heif.MAJOR).toBe(2)
        })

        it('Should return the minor version of Heif', function () {
            expect(Heif.MINOR).toBeDefined()
            expect(Heif.MINOR).not.toBe(null)
            expect(Heif.MINOR).toBe(0)
        })

        it('Should return the patch version of Heif', function () {
            expect(Heif.PATCH).toBeDefined()
            expect(Heif.PATCH).not.toBe(null)
            expect(Heif.PATCH).toBe(0)
        })

        it('Should return the code name of Heif', function () {
            expect(Heif.CODE_NAME).toBeDefined()
            expect(Heif.CODE_NAME).not.toBe(null)
            expect(Heif.CODE_NAME).toBe("ADRIATIC SEA")
        })
 
})