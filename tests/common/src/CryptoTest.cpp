/**
Copyright (c) 2022 Roman Katuntsev <sbkarr@stappler.org>
Copyright (c) 2023 Stappler LLC <admin@stappler.dev>

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
THE SOFTWARE.
**/

#include "SPCommon.h"
#include "SPString.h"
#include "SPTime.h"
#include "SPValid.h"
#include "Test.h"

#if MODULE_STAPPLER_CRYPTO

#include "SPCrypto.h"

namespace stappler::app::test {

static constexpr StringView s_PKCS1PemKey(
R"PrivKey(-----BEGIN RSA PRIVATE KEY-----
MIIEpAIBAAKCAQEAqhnGVkDHG+m2Cm+aFs9gsKRGop9wFyj7KVM9YwkaNtYodFHg
p3k9ybsQopx9VpIC5Ov9ah6oUP/Iyc5Fq0WhCNryi1OGOPHk7J+i3s6exfY6zeaW
/kACtFlpzo6n1a+O2iDjBBpfcNvoCFbpahOlK+zOLzeSlfhamRQWpKacgdIVyLUW
QqUiNBhcH6rTPBLmRC0yFP85XaGJWy/iKXrYLCRmuEDOXP9QWcjpjXJk1+Esuch1
b1oe9ujCls9WAWXwbgMvRX/39qPGVsGs2/PQkKCZTVEESBvmpj9BIfiEFfHt3bw4
+xCVnAGVIceAg/TPXSsk2KJMxgya1k2oH9vlPQIDAQABAoIBAA9/hAcq0BlhIhFx
rqftDXcQA1GLgP0ojOMXLvd9hlJrER+GsEHXYEOnlByaG9S4k/lDQ9Zt5FG7pGc4
BeKqLZcZdSeFnNabujW8VmzCMEwteEPMrS2NgSbpI09LHHackBbxZSMOu2sUgN/K
+hRh+tbK338E1gnCVDrbTjQtQVhAjky3BGB746nzn1YUr2RLGj4cnWQn4BgdnmEm
myuJbz2UU7ual4GiNDXuJnRwyXOimWoPbYQ6uIdnrd0sPcAy7Qsy8xP8XeW6xjR0
lnLXCoHnt8RnDZecZjuZITd1EsIf6Vo+CYtYoreZOyeM0uywK+qY9tl5+TT9hdb2
9yJ9NP8CgYEAwCbYH1/n9I/PmYHZ4BrT8LIvHbF39slXN5Kf56dtKmpMTzWz5vmV
sFSU8EZC6QLiioBi+i/uLleiXayvdKrMbzCzpfRO2dnWbM9fDodSY++BNrfkpNg+
bs5xoQ1ZXlpsZnSreo1d15uHXXp03YYbHBQ9aQbVqrgC6UV1LEIFCA8CgYEA4p8v
LuJxFlNoKppFaiGPgiQCOyQoggKBwt68QJWeziH0Y4MNC61zraJAbRDw7+RIoxFT
CSZNZ0uoO4GA5bMXAuq5UBljZeQ9ElYGte9jG7d07h87JBDH7monscpW5c0Ddhj4
OtX09qe9hETzy9Wf4cwfzAVTXBhlZCyJ/nVv0fMCgYEAhUZsANb0e5yD0WVPSTFS
b1AnfeDp5DIiXFlGr9Zg0VqJMyd8cGgMexEvfLg/EH7/wjDqdb1o5pvB8lckGzec
NEMDV8fsKQ3+u9nZhKo3azlj1iAWZn/WTeTCy4IGynrNVQL3LTmALVuiuCOHi17I
zuD69y3WZWLcIhAbBWeFNeECgYAm1f7A+x9EIzEDtAJXSL8OH1uehPjIJuPT4FXE
f5+CVcFK2GeA682aIOcHidKwWZ+1Cj//nme/XvDKmcvcEU/NOSpetqsZB/8LSGDI
BukmE89fC00YRiPtEJYS2sj/gp2oPGk1s/rR1jcdFq/s64QFrvR4AyWg77FYdGWF
jNyHzwKBgQC+IDISl0f9m6r5m9x3QPL15i+QThez6dCl21RfB95zgyJ77uEiuuVJ
Ajb8m9g9OMD6eiNKaO3JTzG+tBh2asnJU7qLis2eRv8x/6ZDW0l2/T+XK8DY7SvG
/xOgYeYYqjPa9brFJTQ+nOAO9pWLYVjcCxt2BqYFsR4exm5UKgGoPw==
-----END RSA PRIVATE KEY-----
)PrivKey");

static constexpr StringView s_PKCS8PemKey(
R"PrivKey(-----BEGIN PRIVATE KEY-----
MIIEvgIBADANBgkqhkiG9w0BAQEFAASCBKgwggSkAgEAAoIBAQCqGcZWQMcb6bYK
b5oWz2CwpEain3AXKPspUz1jCRo21ih0UeCneT3JuxCinH1WkgLk6/1qHqhQ/8jJ
zkWrRaEI2vKLU4Y48eTsn6Lezp7F9jrN5pb+QAK0WWnOjqfVr47aIOMEGl9w2+gI
VulqE6Ur7M4vN5KV+FqZFBakppyB0hXItRZCpSI0GFwfqtM8EuZELTIU/zldoYlb
L+IpetgsJGa4QM5c/1BZyOmNcmTX4Sy5yHVvWh726MKWz1YBZfBuAy9Ff/f2o8ZW
wazb89CQoJlNUQRIG+amP0Eh+IQV8e3dvDj7EJWcAZUhx4CD9M9dKyTYokzGDJrW
Tagf2+U9AgMBAAECggEAD3+EByrQGWEiEXGup+0NdxADUYuA/SiM4xcu932GUmsR
H4awQddgQ6eUHJob1LiT+UND1m3kUbukZzgF4qotlxl1J4Wc1pu6NbxWbMIwTC14
Q8ytLY2BJukjT0scdpyQFvFlIw67axSA38r6FGH61srffwTWCcJUOttONC1BWECO
TLcEYHvjqfOfVhSvZEsaPhydZCfgGB2eYSabK4lvPZRTu5qXgaI0Ne4mdHDJc6KZ
ag9thDq4h2et3Sw9wDLtCzLzE/xd5brGNHSWctcKgee3xGcNl5xmO5khN3USwh/p
Wj4Ji1iit5k7J4zS7LAr6pj22Xn5NP2F1vb3In00/wKBgQDAJtgfX+f0j8+Zgdng
GtPwsi8dsXf2yVc3kp/np20qakxPNbPm+ZWwVJTwRkLpAuKKgGL6L+4uV6JdrK90
qsxvMLOl9E7Z2dZsz18Oh1Jj74E2t+Sk2D5uznGhDVleWmxmdKt6jV3Xm4ddenTd
hhscFD1pBtWquALpRXUsQgUIDwKBgQDiny8u4nEWU2gqmkVqIY+CJAI7JCiCAoHC
3rxAlZ7OIfRjgw0LrXOtokBtEPDv5EijEVMJJk1nS6g7gYDlsxcC6rlQGWNl5D0S
Vga172Mbt3TuHzskEMfuaiexylblzQN2GPg61fT2p72ERPPL1Z/hzB/MBVNcGGVk
LIn+dW/R8wKBgQCFRmwA1vR7nIPRZU9JMVJvUCd94OnkMiJcWUav1mDRWokzJ3xw
aAx7ES98uD8Qfv/CMOp1vWjmm8HyVyQbN5w0QwNXx+wpDf672dmEqjdrOWPWIBZm
f9ZN5MLLggbKes1VAvctOYAtW6K4I4eLXsjO4Pr3LdZlYtwiEBsFZ4U14QKBgCbV
/sD7H0QjMQO0AldIvw4fW56E+Mgm49PgVcR/n4JVwUrYZ4DrzZog5weJ0rBZn7UK
P/+eZ79e8MqZy9wRT805Kl62qxkH/wtIYMgG6SYTz18LTRhGI+0QlhLayP+Cnag8
aTWz+tHWNx0Wr+zrhAWu9HgDJaDvsVh0ZYWM3IfPAoGBAL4gMhKXR/2bqvmb3HdA
8vXmL5BOF7Pp0KXbVF8H3nODInvu4SK65UkCNvyb2D04wPp6I0po7clPMb60GHZq
yclTuouKzZ5G/zH/pkNbSXb9P5crwNjtK8b/E6Bh5hiqM9r1usUlND6c4A72lYth
WNwLG3YGpgWxHh7GblQqAag/
-----END PRIVATE KEY-----
)PrivKey");

static constexpr StringView s_PKCS1DerKey(
R"PrivKey(
MIIEpAIBAAKCAQEAqhnGVkDHG+m2Cm+aFs9gsKRGop9wFyj7KVM9YwkaNtYodFHg
p3k9ybsQopx9VpIC5Ov9ah6oUP/Iyc5Fq0WhCNryi1OGOPHk7J+i3s6exfY6zeaW
/kACtFlpzo6n1a+O2iDjBBpfcNvoCFbpahOlK+zOLzeSlfhamRQWpKacgdIVyLUW
QqUiNBhcH6rTPBLmRC0yFP85XaGJWy/iKXrYLCRmuEDOXP9QWcjpjXJk1+Esuch1
b1oe9ujCls9WAWXwbgMvRX/39qPGVsGs2/PQkKCZTVEESBvmpj9BIfiEFfHt3bw4
+xCVnAGVIceAg/TPXSsk2KJMxgya1k2oH9vlPQIDAQABAoIBAA9/hAcq0BlhIhFx
rqftDXcQA1GLgP0ojOMXLvd9hlJrER+GsEHXYEOnlByaG9S4k/lDQ9Zt5FG7pGc4
BeKqLZcZdSeFnNabujW8VmzCMEwteEPMrS2NgSbpI09LHHackBbxZSMOu2sUgN/K
+hRh+tbK338E1gnCVDrbTjQtQVhAjky3BGB746nzn1YUr2RLGj4cnWQn4BgdnmEm
myuJbz2UU7ual4GiNDXuJnRwyXOimWoPbYQ6uIdnrd0sPcAy7Qsy8xP8XeW6xjR0
lnLXCoHnt8RnDZecZjuZITd1EsIf6Vo+CYtYoreZOyeM0uywK+qY9tl5+TT9hdb2
9yJ9NP8CgYEAwCbYH1/n9I/PmYHZ4BrT8LIvHbF39slXN5Kf56dtKmpMTzWz5vmV
sFSU8EZC6QLiioBi+i/uLleiXayvdKrMbzCzpfRO2dnWbM9fDodSY++BNrfkpNg+
bs5xoQ1ZXlpsZnSreo1d15uHXXp03YYbHBQ9aQbVqrgC6UV1LEIFCA8CgYEA4p8v
LuJxFlNoKppFaiGPgiQCOyQoggKBwt68QJWeziH0Y4MNC61zraJAbRDw7+RIoxFT
CSZNZ0uoO4GA5bMXAuq5UBljZeQ9ElYGte9jG7d07h87JBDH7monscpW5c0Ddhj4
OtX09qe9hETzy9Wf4cwfzAVTXBhlZCyJ/nVv0fMCgYEAhUZsANb0e5yD0WVPSTFS
b1AnfeDp5DIiXFlGr9Zg0VqJMyd8cGgMexEvfLg/EH7/wjDqdb1o5pvB8lckGzec
NEMDV8fsKQ3+u9nZhKo3azlj1iAWZn/WTeTCy4IGynrNVQL3LTmALVuiuCOHi17I
zuD69y3WZWLcIhAbBWeFNeECgYAm1f7A+x9EIzEDtAJXSL8OH1uehPjIJuPT4FXE
f5+CVcFK2GeA682aIOcHidKwWZ+1Cj//nme/XvDKmcvcEU/NOSpetqsZB/8LSGDI
BukmE89fC00YRiPtEJYS2sj/gp2oPGk1s/rR1jcdFq/s64QFrvR4AyWg77FYdGWF
jNyHzwKBgQC+IDISl0f9m6r5m9x3QPL15i+QThez6dCl21RfB95zgyJ77uEiuuVJ
Ajb8m9g9OMD6eiNKaO3JTzG+tBh2asnJU7qLis2eRv8x/6ZDW0l2/T+XK8DY7SvG
/xOgYeYYqjPa9brFJTQ+nOAO9pWLYVjcCxt2BqYFsR4exm5UKgGoPw==
)PrivKey");

static constexpr StringView s_PKCS8DerKey(
R"PrivKey(MIIEvgIBADANBgkqhkiG9w0BAQEFAASCBKgwggSkAgEAAoIBAQCqGcZWQMcb6bYK
b5oWz2CwpEain3AXKPspUz1jCRo21ih0UeCneT3JuxCinH1WkgLk6/1qHqhQ/8jJ
zkWrRaEI2vKLU4Y48eTsn6Lezp7F9jrN5pb+QAK0WWnOjqfVr47aIOMEGl9w2+gI
VulqE6Ur7M4vN5KV+FqZFBakppyB0hXItRZCpSI0GFwfqtM8EuZELTIU/zldoYlb
L+IpetgsJGa4QM5c/1BZyOmNcmTX4Sy5yHVvWh726MKWz1YBZfBuAy9Ff/f2o8ZW
wazb89CQoJlNUQRIG+amP0Eh+IQV8e3dvDj7EJWcAZUhx4CD9M9dKyTYokzGDJrW
Tagf2+U9AgMBAAECggEAD3+EByrQGWEiEXGup+0NdxADUYuA/SiM4xcu932GUmsR
H4awQddgQ6eUHJob1LiT+UND1m3kUbukZzgF4qotlxl1J4Wc1pu6NbxWbMIwTC14
Q8ytLY2BJukjT0scdpyQFvFlIw67axSA38r6FGH61srffwTWCcJUOttONC1BWECO
TLcEYHvjqfOfVhSvZEsaPhydZCfgGB2eYSabK4lvPZRTu5qXgaI0Ne4mdHDJc6KZ
ag9thDq4h2et3Sw9wDLtCzLzE/xd5brGNHSWctcKgee3xGcNl5xmO5khN3USwh/p
Wj4Ji1iit5k7J4zS7LAr6pj22Xn5NP2F1vb3In00/wKBgQDAJtgfX+f0j8+Zgdng
GtPwsi8dsXf2yVc3kp/np20qakxPNbPm+ZWwVJTwRkLpAuKKgGL6L+4uV6JdrK90
qsxvMLOl9E7Z2dZsz18Oh1Jj74E2t+Sk2D5uznGhDVleWmxmdKt6jV3Xm4ddenTd
hhscFD1pBtWquALpRXUsQgUIDwKBgQDiny8u4nEWU2gqmkVqIY+CJAI7JCiCAoHC
3rxAlZ7OIfRjgw0LrXOtokBtEPDv5EijEVMJJk1nS6g7gYDlsxcC6rlQGWNl5D0S
Vga172Mbt3TuHzskEMfuaiexylblzQN2GPg61fT2p72ERPPL1Z/hzB/MBVNcGGVk
LIn+dW/R8wKBgQCFRmwA1vR7nIPRZU9JMVJvUCd94OnkMiJcWUav1mDRWokzJ3xw
aAx7ES98uD8Qfv/CMOp1vWjmm8HyVyQbN5w0QwNXx+wpDf672dmEqjdrOWPWIBZm
f9ZN5MLLggbKes1VAvctOYAtW6K4I4eLXsjO4Pr3LdZlYtwiEBsFZ4U14QKBgCbV
/sD7H0QjMQO0AldIvw4fW56E+Mgm49PgVcR/n4JVwUrYZ4DrzZog5weJ0rBZn7UK
P/+eZ79e8MqZy9wRT805Kl62qxkH/wtIYMgG6SYTz18LTRhGI+0QlhLayP+Cnag8
aTWz+tHWNx0Wr+zrhAWu9HgDJaDvsVh0ZYWM3IfPAoGBAL4gMhKXR/2bqvmb3HdA
8vXmL5BOF7Pp0KXbVF8H3nODInvu4SK65UkCNvyb2D04wPp6I0po7clPMb60GHZq
yclTuouKzZ5G/zH/pkNbSXb9P5crwNjtK8b/E6Bh5hiqM9r1usUlND6c4A72lYth
WNwLG3YGpgWxHh7GblQqAag/)PrivKey");

static constexpr StringView s_PubPemKey(
R"PubKey(-----BEGIN PUBLIC KEY-----
MIIBIjANBgkqhkiG9w0BAQEFAAOCAQ8AMIIBCgKCAQEAysyJ6uo23iwchAW9/TqZ
9aLM9uAN76hFIE9h1ifxLGnYgrck5PRSFPbIcayjv/OC6Qyr48LhPh+mLhIfekEp
EFC6L9FjEPZ2UnkBpX7FayMTEnpc+r2ExP8c6ZBS+MIxVuMEINWFexcUX0O4McpT
mf0gUsCC9JwI1aplIsG8rxU0O1FQws7F4RNukkPIhVdLyxxyzdear6Y+ad20fC57
uF4XDvswtOmAahMv7YH2pm3HwNr8d/bOh5ScJwVmVPyCj/Fcmh1swplYNOzPGbLJ
T2H5JHvkDQIWOBo1HwAIbrDeBY5qbWmov8Q9erz5ARS1thepfo7Fa6IVzTHo0RGN
NwIDAQAB
-----END PUBLIC KEY-----
)PubKey");

static constexpr StringView s_PubDerKey(
R"PubKey(MIIBIjANBgkqhkiG9w0BAQEFAAOCAQ8AMIIBCgKCAQEAysyJ6uo23iwchAW9/TqZ
9aLM9uAN76hFIE9h1ifxLGnYgrck5PRSFPbIcayjv/OC6Qyr48LhPh+mLhIfekEp
EFC6L9FjEPZ2UnkBpX7FayMTEnpc+r2ExP8c6ZBS+MIxVuMEINWFexcUX0O4McpT
mf0gUsCC9JwI1aplIsG8rxU0O1FQws7F4RNukkPIhVdLyxxyzdear6Y+ad20fC57
uF4XDvswtOmAahMv7YH2pm3HwNr8d/bOh5ScJwVmVPyCj/Fcmh1swplYNOzPGbLJ
T2H5JHvkDQIWOBo1HwAIbrDeBY5qbWmov8Q9erz5ARS1thepfo7Fa6IVzTHo0RGN
NwIDAQAB)PubKey");

static constexpr StringView s_PubSsh(
R"PubKey(ssh-rsa AAAAB3NzaC1yc2EAAAADAQABAAABgQDe4jx0T5fws+WIGsio1bOlYzdab+VvPv+MQNuvQ8gK8iE0ioYEIUVsxGNu3iJHpo0K6H/Y8F6aNOfx2etbDgpxYyWbIZ/y1XtqklXQQ7lW2NzPDlu3ioDC57KPUaBvMFU0fbpx7AGlYBhnow/jYAWSmi2MywcbErpLJv1Wgp+mjsmNZ6VhLUlaO/3aPVSl9Df+dizp3gQLP2ut6RBIyB704aYdbaHwrTxmvLn9Gb9GaCMRM+MKfCzYv6yJVLFGK/C7yLSw/kPquseAaqu99XOHL6opaRx9ezdk0KijpM+Ow/gYf7T3maCMPDpf33tiQaqQLXZgUHsXckSWADFT5m/q7+0Ic0DHsxiAH8y+pEs4lWVS2f7jGWjzVtXXkN8UM+LDfxb5Ex6bbAl/KuCTjfULGBOg6AAi10ak0MbF8sDWrBSpY6eCb9n8cnXaNH7szLweqh6F5oceh1dVuO8WILCQdWr11DY5KCzONjhXcFAYPoFugQBOzc4RomBN6DUoca0= sbkarr@sbkarr-home-main)PubKey");

static constexpr StringView s_PubSshPem(
R"PubKey(-----BEGIN RSA PUBLIC KEY-----
MIIBigKCAYEA3uI8dE+X8LPliBrIqNWzpWM3Wm/lbz7/jEDbr0PICvIhNIqGBCFF
bMRjbt4iR6aNCuh/2PBemjTn8dnrWw4KcWMlmyGf8tV7apJV0EO5Vtjczw5bt4qA
wueyj1GgbzBVNH26cewBpWAYZ6MP42AFkpotjMsHGxK6Syb9VoKfpo7JjWelYS1J
Wjv92j1UpfQ3/nYs6d4ECz9rrekQSMge9OGmHW2h8K08Zry5/Rm/RmgjETPjCnws
2L+siVSxRivwu8i0sP5D6rrHgGqrvfVzhy+qKWkcfXs3ZNCoo6TPjsP4GH+095mg
jDw6X997YkGqkC12YFB7F3JElgAxU+Zv6u/tCHNAx7MYgB/MvqRLOJVlUtn+4xlo
81bV15DfFDPiw38W+RMem2wJfyrgk431CxgToOgAItdGpNDGxfLA1qwUqWOngm/Z
/HJ12jR+7My8HqoeheaHHodXVbjvFiCwkHVq9dQ2OSgszjY4V3BQGD6BboEATs3O
EaJgTeg1KHGtAgMBAAE=
-----END RSA PUBLIC KEY-----
)PubKey");

SPUNUSED static constexpr StringView s_PrivSsh(
R"PrivKey(-----BEGIN OPENSSH PRIVATE KEY-----
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
-----END OPENSSH PRIVATE KEY-----
)PrivKey");

SPUNUSED static constexpr StringView s_GostPrivKey(
R"GostKey(-----BEGIN PRIVATE KEY-----
MGgCAQAwIQYIKoUDBwEBAQIwFQYJKoUDBwECAQIBBggqhQMHAQECAwRAzyYtvsjl
i2APv6u/DaGuR5t5tek8+rERoIE/hI9AuMu0JyIMo4Hd26NIMXsR9NODV+Nh8a9L
E8OCKW9N7veiLA==
-----END PRIVATE KEY-----
)GostKey");

SPUNUSED static constexpr StringView s_GostPubKey(
R"GostKey(-----BEGIN PUBLIC KEY-----
MIGqMCEGCCqFAwcBAQECMBUGCSqFAwcBAgECAQYIKoUDBwEBAgMDgYQABIGALi2S
+Um6ZifvkpmdmZU2cugiw147adXKI8mcHoffwLY7Xo/q/9GiftqZzu56p/VMzyRm
hF/x7IotkwHA6WrFs680jwIEWxS7vzUiBWnn+angRTNTsWRXzCfkLpHuA7SopeNq
Hnk1VdlqrfqjyV13si/4BEYpYmC75paNA5opHYg=
-----END PUBLIC KEY-----
)GostKey");



static bool CryptoTest_genkey(std::ostream &stream, crypto::Backend b, crypto::KeyType type) {
	crypto::PrivateKey key(b);

	if (!key.isGenerateSupported(type)) {
		return true;
	}

	key.generate(crypto::KeyBits::_2048, type);

	String pemPKCS1;
	String derPKCS1;

	String pemPKCS8;
	String derPKCS8;

	if (type == crypto::KeyType::RSA && key.isSupported(crypto::KeyFormat::PKCS1)) {
		key.exportPem([&] (BytesView data) {
			pemPKCS1 = StringView(data.toStringView()).str<Interface>();
		}, crypto::KeyFormat::PKCS1);

		key.exportDer([&] (BytesView data) {
			StringStream derKeyData;
			derKeyData << "-----BEGIN RSA PRIVATE KEY-----\n";
			size_t counter = 0;
			base64::encode([&] (char c) {
				if (counter ++ > 63) {
					derKeyData << "\n";
					counter = 1;
				}
				derKeyData << c;
			}, data);
			derKeyData << "\n-----END RSA PRIVATE KEY-----\n";
			derPKCS1 = derKeyData.str();
		}, crypto::KeyFormat::PKCS1);

		if (pemPKCS1.empty() || derPKCS1.empty() || pemPKCS1 != derPKCS1) {
			std::cout << pemPKCS1 << "\n\n" << derPKCS1 << "\n";
			stream << "PKCS1: DER != PEM";
			return false;
		}
	}

	if (key.isSupported(crypto::KeyFormat::PKCS8)) {
		key.exportPem([&] (BytesView data) {
			pemPKCS8 = StringView(data.toStringView()).str<Interface>();
		}, crypto::KeyFormat::PKCS8);

		key.exportDer([&] (BytesView data) {
			StringStream derKeyData;
			StringStream encodedData;
			derKeyData << "-----BEGIN PRIVATE KEY-----\n";
			size_t counter = 0;
			base64::encode([&] (char c) {
				if (counter ++ > 63) {
					encodedData << "\n";
					counter = 1;
				}
				encodedData << c;
			}, data);
			derKeyData << encodedData.str() << "\n-----END PRIVATE KEY-----\n";

			auto tmp = base64::decode<Interface>(encodedData.str());
			if (tmp != data) {
				stream << "Invalid base64 decoder;";
			}

			derPKCS8 = derKeyData.str();
		}, crypto::KeyFormat::PKCS8);

		if (pemPKCS8 != derPKCS8) {
			stream << "PKCS8: DER != PEM";
			return false;
		}
	}

	crypto::PublicKey pub(key);

	String pemPub;
	String derPub;

	pub.exportPem([&] (BytesView data) {
		pemPub = StringView(data.toStringView()).str<Interface>();
	});

	pub.exportDer([&] (BytesView data) {
		StringStream derKeyData;
		derKeyData << "-----BEGIN PUBLIC KEY-----\n";
		size_t counter = 0;
		base64::encode([&] (char c) {
			if (counter ++ > 63) {
				derKeyData << "\n";
				counter = 1;
			}
			derKeyData << c;
		}, data);
		derKeyData << "\n-----BEGIN PUBLIC KEY-----\n";
		derPub = derKeyData.str();
	});

	if (pemPKCS8 != derPKCS8) {
		stream << "Pub: DER != PEM";
		return false;
	}

	return key;
}

static bool CryptoTest_load(std::ostream &stream, crypto::Backend b) {
	bool success = true;
	crypto::PrivateKey key1(b, BytesView((const uint8_t *)s_PKCS1PemKey.data(), s_PKCS1PemKey.size()));
	if (!key1) {
		return false;
	}

	auto key3Data = base64::decode<Interface>(s_PKCS1DerKey);
	auto key4Data = base64::decode<Interface>(s_PKCS8DerKey);

	if (key1.isSupported(crypto::KeyFormat::PKCS8)) {
		key1.exportDer([&] (BytesView data) {
			if (BytesView(key4Data) != data) {
				stream << " PKCS8 export not match;";
				success = false;
			}
			crypto::PrivateKey tmp(data);
			if (!tmp) {
				stream << " PKCS1 -> Der failed;";
				success = false;
			}
		}, crypto::KeyFormat::PKCS8);
	}

	if (key1.isSupported(crypto::KeyFormat::PKCS1)) {
		key1.exportDer([&] (BytesView data) {
			if (BytesView(key3Data) != data) {
				stream << " PKCS1 export not match;";
				success = false;

				std::cout << "\n";
				size_t counter = 0;
				base64::encode([&] (char c) {
					if (counter ++ > 63) {
						std::cout << "\n";
						counter = 1;
					}
					std::cout << c;
				}, data);

				std::cout << "\nVS\n" << s_PKCS1DerKey << "\n";
			}
			crypto::PrivateKey tmp(data);
			if (!tmp) {
				stream << " PKCS8 -> Der failed;";
				success = false;
			}
		}, crypto::KeyFormat::PKCS1);
	}

	crypto::PrivateKey key2(b, BytesView((const uint8_t *)s_PKCS8PemKey.data(), s_PKCS8PemKey.size()));
	if (!key2) {
		stream << "fail to loaded s_PKCS8PemKey;";
		return false;
	}

	crypto::PrivateKey key3(b, key3Data);
	if (!key3) {
		stream << "fail to loaded key3Data;";
		return false;
	}

	crypto::PrivateKey key4(b, key4Data);
	if (!key4) {
		stream << "fail to loaded key4Data;";
		return false;
	}

	if (key1.isSupported(crypto::KeyFormat::PKCS8)) {
		key1.exportPem([&] (BytesView data) {
			if (s_PKCS8PemKey != StringView(data.toStringView())) {
				stream << " PKCS1 -> PKCS8 != PKCS8;";
				success = false;
			}
		}, crypto::KeyFormat::PKCS8);
	}

	if (key1.isSupported(crypto::KeyFormat::PKCS1)) {
		key2.exportPem([&] (BytesView data) {
			if (s_PKCS1PemKey != StringView(data.toStringView())) {
				stream << " PKCS8 -> PKCS1 != PKCS1;";
				success = false;
			}
		}, crypto::KeyFormat::PKCS1);
	}

	return success;
}

static bool CryptoTest_pubload(std::ostream &stream, crypto::Backend b) {
	bool success = true;
	auto key1Data = base64::decode<Interface>(s_PubDerKey);
	crypto::PublicKey key1(b, s_PubPemKey);
	crypto::PublicKey key2(b, key1Data);

	if (!key1) {
		stream << " fail to load s_PubPemKey;";
		return false;
	}

	if (!key2) {
		stream << " fail to load s_PubDerKey;";
		return false;
	}

	key1.exportPem([&] (BytesView data) {
		if (s_PubPemKey != StringView(data.toStringView())) {
			stream << " PUB1 -> PEM != PUB1(PEM);";
			success = false;
		}
	});

	key1.exportDer([&] (BytesView data) {
		if (key1Data != data) {
			stream << " PUB1 -> DER != PUB2(DER);";
			success = false;
		}
	});

	key2.exportPem([&] (BytesView data) {
		if (s_PubPemKey != StringView(data.toStringView())) {
			stream << " PUB2 -> PEM != PUB1(PEM);";
			success = false;
		}
	});

	key2.exportDer([&] (BytesView data) {
		if (key1Data != data) {
			stream << " PUB2 -> DER != PUB2(DER);";
			success = false;
		}
	});

	return success;
}

static bool CryptoTest_ssh(std::ostream &stream, crypto::Backend b) {
	crypto::PublicKey key1(b);
	key1.importOpenSSH(s_PubSsh);
	if (!key1) {
		return false;
	}

	crypto::PublicKey key2(b, s_PubSshPem);
	if (!key2) {
		return false;
	}

	String key2Data;

	key2.exportPem([&] (BytesView data) {
		key2Data = StringView(data.toStringView()).str<Interface>();
	});

	bool success = false;
	if (key1) {
		key1.exportPem([&] (BytesView data) {
			if (key2Data != StringView(data.toStringView())) {
				stream << "ssh-pubkey != ssh-pem-pubkey;";
				std::cout << key2Data << "\n" << StringView(data.toStringView()) << "\n";
			} else {
				success = true;
			}
		});
	}
	return success;
}

static bool CryptoTest_sign_validate(std::ostream &stream, crypto::Backend b) {
	bool success = false;

	crypto::PrivateKey pk(b, BytesView((const uint8_t *)s_PKCS8PemKey.data(), s_PKCS8PemKey.size()));

	auto pub = pk.exportPublic();

	String signData;

	// follow serenity pkey auth method
	pub.exportDer([&] (BytesView pub) {
		crypto::PublicKey spub(b, pub);
		if (!spub) {
			return;
		}

		pk.sign([&] (BytesView sign) {
			signData = base64::encode<Interface>(data::write<Interface>(data::ValueTemplate<Interface>({
				data::ValueTemplate<Interface>(pub),
				data::ValueTemplate<Interface>(sign)
			})));
		}, pub, crypto::SignAlgorithm::RSA_SHA512);
	});

	if (!signData.empty()) {
		auto data = data::read<Interface>(base64::decode<Interface>(signData));

		BytesView signedKey = data.getBytes(0);
		BytesView signature = data.getBytes(1);

		crypto::PublicKey spub(b, signedKey);

		success = spub.verify(signedKey, signature, crypto::SignAlgorithm::RSA_SHA512);
		if (!success) {
			stream << "verification failed;";
		}
	}

	return success;
}

static bool CryptoTest_gost_sign(std::ostream &stream, crypto::Backend b) {
	crypto::PrivateKey pk(b, BytesView((const uint8_t *)s_GostPrivKey.data(), s_GostPrivKey.size()));
	if (pk.getType() != crypto::KeyType::GOST3410_2012_512) {
		return false;
	}

	String pkStr;
	pk.exportPem([&] (BytesView data) {
		pkStr = StringView(data.toStringView()).str<Interface>();
	});

	crypto::PublicKey pub(b, BytesView((const uint8_t *)s_GostPubKey.data(), s_GostPubKey.size()));
	if (pub.getType() != crypto::KeyType::GOST3410_2012_512) {
		return false;
	}

	Bytes pubStr;
	pub.exportDer([&] (BytesView data) {
		pubStr = data.bytes<Interface>();
	});

	String signData;
	pk.sign([&] (BytesView sign) {
		signData = base64::encode<Interface>(data::write<Interface>(data::ValueTemplate<Interface>({
			data::ValueTemplate<Interface>(pubStr),
			data::ValueTemplate<Interface>(sign)
		})));
	}, pubStr, crypto::SignAlgorithm::GOST_512);

	if (!signData.empty()) {
		auto data = data::read<Interface>(base64::decode<Interface>(signData));

		BytesView signedKey = data.getBytes(0);
		BytesView signature = data.getBytes(1);

		crypto::PublicKey spub(b, signedKey);

		auto success = spub.verify(signedKey, signature, crypto::SignAlgorithm::GOST_512);
		if (!success) {
			stream << "verification failed;";
		}
		return success;
	}

	return false;
}

struct CryptoTest : Test {
	CryptoTest() : Test("CryptoTest") { }

	virtual bool run() override {
		StringStream stream;
		size_t count = 0;
		size_t passed = 0;
		stream << "\n";

		runTest(stream, toString("gost3411-interop"), count, passed, [&] () -> bool {
			auto test = [] (const CoderSource & msg, BytesView result256, BytesView result512) {
				auto v256_1 = crypto::hash256(crypto::Backend::Embedded, msg, crypto::HashFunction::GOST_3411);
				auto v256_2 = crypto::hash256(crypto::Backend::GnuTLS, msg, crypto::HashFunction::GOST_3411);
				auto v256_3 = crypto::hash256(crypto::Backend::OpenSSL, msg, crypto::HashFunction::GOST_3411);

				auto v512_1 = crypto::hash512(crypto::Backend::Embedded, msg, crypto::HashFunction::GOST_3411);
				auto v512_2 = crypto::hash512(crypto::Backend::GnuTLS, msg, crypto::HashFunction::GOST_3411);
				auto v512_3 = crypto::hash512(crypto::Backend::OpenSSL, msg, crypto::HashFunction::GOST_3411);

				return v256_1 == v256_2 && v256_2 == v256_3 && v256_3 == result256
						&& v512_1 == v512_2 && v512_2 == v512_3 && v512_3 == result512;
			};

			return test(
					"012345678901234567890123456789012345678901234567890123456789012",
					base16::decode<Interface>("9d151eefd8590b89daa6ba6cb74af9275dd051026bb149a452fd84e5e57b5500"),
					base16::decode<Interface>("1b54d01a4af5b9d5cc3d86d68d285462b19abc2475222f35c085122be4ba1ffa00ad30f8767b3a82384c6574f024c311e2a481332b08ef7f41797891c1646f48")
					)
				&& test(
					base16::decode<Interface>("D1E520E2E5F2F0E82C20D1F2F0E8E1EEE6E820E2EDF3F6E82C20E2E5FEF2FA20F120ECEEF0FF20F1F2F0E5EBE0ECE820EDE020F5F0E0E1F0FBFF20EFEBFAEAFB20C8E3EEF0E5E2FB"),
					base16::decode<Interface>("9dd2fe4e90409e5da87f53976d7405b0c0cac628fc669a741d50063c557e8f50"),
					base16::decode<Interface>("1e88e62226bfca6f9994f1f2d51569e0daf8475a3b0fe61a5300eee46d961376035fe83549ada2b8620fcd7c496ce5b33f0cb9dddc2b6460143b03dabac9fb28")
					)
				&& test(
					base16::decode<Interface>("00000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000"),
					base16::decode<Interface>("df1fda9ce83191390537358031db2ecaa6aa54cd0eda241dc107105e13636b95"),
					base16::decode<Interface>("b0fd29ac1b0df441769ff3fdb8dc564df67721d6ac06fb28ceffb7bbaa7948c6c014ac999235b58cb26fb60fb112a145d7b4ade9ae566bf2611402c552d20db7")
					)
				&& test(
					"",
					base16::decode<Interface>("3f539a213e97c802cc229d474c6aa32a825a360b2a933a949fd925208d9ce1bb"),
					base16::decode<Interface>("8e945da209aa869f0455928529bcae4679e9873ab707b55315f56ceb98bef0a7362f715528356ee83cda5f2aac4c6ad2ba3a715c1bcd81cb8e9f90bf4c1c1a8a")
					);
		});

		runTest(stream, toString("ssh-2-interop"), count, passed, [&] () -> bool {
			auto sha256_1 = crypto::hash256(crypto::Backend::MbedTLS, CoderSource(s_PKCS8PemKey), crypto::HashFunction::SHA_2);
			auto sha256_2 = crypto::hash256(crypto::Backend::GnuTLS, CoderSource(s_PKCS8PemKey), crypto::HashFunction::SHA_2);
			auto sha256_3 = crypto::hash256(crypto::Backend::OpenSSL, CoderSource(s_PKCS8PemKey), crypto::HashFunction::SHA_2);
			auto sha256_4 = crypto::hash256(crypto::Backend::Embedded, CoderSource(s_PKCS8PemKey), crypto::HashFunction::SHA_2);

			auto sha512_1 = crypto::hash512(crypto::Backend::MbedTLS, CoderSource(s_PKCS8PemKey), crypto::HashFunction::SHA_2);
			auto sha512_2 = crypto::hash512(crypto::Backend::GnuTLS, CoderSource(s_PKCS8PemKey), crypto::HashFunction::SHA_2);
			auto sha512_3 = crypto::hash512(crypto::Backend::OpenSSL, CoderSource(s_PKCS8PemKey), crypto::HashFunction::SHA_2);
			auto sha512_4 = crypto::hash512(crypto::Backend::Embedded, CoderSource(s_PKCS8PemKey), crypto::HashFunction::SHA_2);

			return sha256_1 == sha256_2 && sha256_3 == sha256_4 && sha256_2 == sha256_3
					&& sha512_1 == sha512_2 && sha512_3 == sha512_4 && sha512_2 == sha512_3;
		});

		runTest(stream, toString("block-cipher-gost"), count, passed, [&] () -> bool {
			bool success = true;

			crypto::PrivateKey opensslPk(crypto::Backend::OpenSSL, s_GostPrivKey);
			auto opensslGostKey = crypto::makeBlockKey(opensslPk, s_GostPrivKey, crypto::BlockCipher::Gost3412_2015_CTR_ACPKM);

			Bytes opensslEncoded;
			crypto::encryptBlock(crypto::Backend::OpenSSL, opensslGostKey, s_GostPrivKey, [&] (BytesView buf) {
				opensslEncoded = buf.bytes<Interface>();
			});

			crypto::decryptBlock(crypto::Backend::OpenSSL, opensslGostKey, opensslEncoded, [&] (BytesView buf) {
				if (buf != BytesView(s_GostPrivKey)) {
					stream << "OpenSSL: GOST decryption failed";
					success = false;
				}
			});

#ifdef MODULE_STAPPLER_CRYPTO_GNUTLS
			crypto::PrivateKey gnutlsPk(crypto::Backend::GnuTLS, s_GostPrivKey);
			auto gnutlsGostKey = crypto::makeBlockKey(gnutlsPk, s_GostPrivKey, crypto::BlockCipher::Gost3412_2015_CTR_ACPKM);

			if (gnutlsGostKey != opensslGostKey) {
				stream << "Keys not equal: ";
				base16::encode(stream, opensslGostKey.data);
				stream << " vs. ";
				base16::encode(stream, gnutlsGostKey.data);
				return false;
			}

			Bytes gnutlsEncoded;
			crypto::encryptBlock(crypto::Backend::GnuTLS, gnutlsGostKey, s_GostPrivKey, [&] (BytesView buf) {
				gnutlsEncoded = buf.bytes<Interface>();
			});

			if constexpr (crypto::SafeBlockEncoding) {
				if (opensslEncoded != gnutlsEncoded) {
					return false;
				}
			}
			crypto::decryptBlock(crypto::Backend::GnuTLS, gnutlsGostKey, gnutlsEncoded, [&] (BytesView buf) {
				if (buf != BytesView(s_GostPrivKey)) {
					stream << "GnuTLS: GOST decryption failed";
					success = false;
				}
			});

			crypto::decryptBlock(crypto::Backend::GnuTLS, opensslGostKey, opensslEncoded, [&] (BytesView buf) {
				if (buf != BytesView(s_GostPrivKey)) {
					stream << "GnuTLS: GOST decryption failed";
					success = false;
				}
			});

			crypto::decryptBlock(crypto::Backend::OpenSSL, gnutlsGostKey, gnutlsEncoded, [&] (BytesView buf) {
				if (buf != BytesView(s_GostPrivKey)) {
					stream << "OpenSSL: GOST decryption failed";
					success = false;
				}
			});
#endif

			return success;
		});

		runTest(stream, toString("block-cipher-gost+RSA"), count, passed, [&] () -> bool {
			crypto::PrivateKey opensslPk(crypto::Backend::OpenSSL, s_PKCS8PemKey);
			auto opensslGostKey = crypto::makeBlockKey(opensslPk, s_PKCS8PemKey, crypto::BlockCipher::Gost3412_2015_CTR_ACPKM);

			Bytes opensslEncoded;
			crypto::encryptBlock(crypto::Backend::OpenSSL, opensslGostKey, s_PKCS8PemKey, [&] (BytesView buf) {
				opensslEncoded = buf.bytes<Interface>();
			});

			bool success = true;

			crypto::decryptBlock(crypto::Backend::OpenSSL, opensslGostKey, opensslEncoded, [&] (BytesView buf) {
				if (buf != BytesView((const uint8_t *)s_PKCS8PemKey.data(), s_PKCS8PemKey.size())) {
					stream << "OpenSSL: GOST decryption failed";
					success = false;
				}
			});

#ifdef MODULE_STAPPLER_CRYPTO_GNUTLS
			crypto::PrivateKey gnutlsPk(crypto::Backend::GnuTLS, s_PKCS8PemKey);
			auto gnutlsGostKey = crypto::makeBlockKey(gnutlsPk, s_PKCS8PemKey, crypto::BlockCipher::Gost3412_2015_CTR_ACPKM);

			if (opensslGostKey != gnutlsGostKey) {
				return false;
			}

			Bytes gnutlsEncoded;

			crypto::encryptBlock(crypto::Backend::GnuTLS, gnutlsGostKey, s_PKCS8PemKey, [&] (BytesView buf) {
				gnutlsEncoded = buf.bytes<Interface>();
			});

			if constexpr (crypto::SafeBlockEncoding) {
				if (opensslEncoded != gnutlsEncoded) {
					return false;
				}
			}

			crypto::decryptBlock(crypto::Backend::GnuTLS, gnutlsGostKey, gnutlsEncoded, [&] (BytesView buf) {
				if (buf != BytesView((const uint8_t *)s_PKCS8PemKey.data(), s_PKCS8PemKey.size())) {
					stream << "GnuTLS: GOST decryption failed";
					success = false;
				}
			});

			crypto::decryptBlock(crypto::Backend::GnuTLS, gnutlsGostKey, opensslEncoded, [&] (BytesView buf) {
				if (buf != BytesView((const uint8_t *)s_PKCS8PemKey.data(), s_PKCS8PemKey.size())) {
					stream << "GnuTLS: GOST-cross decryption failed";
					success = false;
				}
			});

			crypto::decryptBlock(crypto::Backend::OpenSSL, opensslGostKey, gnutlsEncoded, [&] (BytesView buf) {
				if (buf != BytesView((const uint8_t *)s_PKCS8PemKey.data(), s_PKCS8PemKey.size())) {
					stream << "OpenSSL: GOST-cross decryption failed";
					success = false;
				}
			});
#endif

			return success;
		});

		runTest(stream, toString("block-cipher-aes"), count, passed, [&] () -> bool {
			crypto::PrivateKey mbedtlsPk(crypto::Backend::MbedTLS, s_PKCS8PemKey);
			auto mbedtlsAesKey = crypto::makeBlockKey(mbedtlsPk, s_PKCS8PemKey, crypto::BlockCipher::AES_CBC);

			crypto::PrivateKey opensslPk(crypto::Backend::OpenSSL, s_PKCS8PemKey);
			auto opensslAesKey = crypto::makeBlockKey(opensslPk, s_PKCS8PemKey, crypto::BlockCipher::AES_CBC);

			if (opensslAesKey != mbedtlsAesKey) {
				return false;
			}

			bool success = true;

			Bytes opensslEncoded;
			Bytes mbedtlsEncoded;

#ifdef MODULE_STAPPLER_CRYPTO_GNUTLS
			crypto::PrivateKey gnutlsPk(crypto::Backend::GnuTLS, s_PKCS8PemKey);
			auto gnutlsAesKey = crypto::makeBlockKey(gnutlsPk, s_PKCS8PemKey, crypto::BlockCipher::AES_CBC);

			if (opensslAesKey != gnutlsAesKey) {
				return false;
			}

			if (mbedtlsAesKey != gnutlsAesKey) {
				return false;
			}

			Bytes gnutlsEncoded;

			crypto::encryptBlock(crypto::Backend::GnuTLS, gnutlsAesKey, s_PKCS8PemKey, [&] (BytesView buf) {
				gnutlsEncoded = buf.bytes<Interface>();
			});

			crypto::decryptBlock(crypto::Backend::GnuTLS, gnutlsAesKey, gnutlsEncoded, [&] (BytesView buf) {
				if (buf != BytesView((const uint8_t *)s_PKCS8PemKey.data(), s_PKCS8PemKey.size())) {
					stream << "GnuTLS: AES decryption failed";
					success = false;
				}
			});
#endif

			crypto::encryptBlock(crypto::Backend::OpenSSL, opensslAesKey, s_PKCS8PemKey, [&] (BytesView buf) {
				opensslEncoded = buf.bytes<Interface>();
			});

			crypto::decryptBlock(crypto::Backend::OpenSSL, opensslAesKey, opensslEncoded, [&] (BytesView buf) {
				if (buf.bytes<Interface>() != BytesView((const uint8_t *)s_PKCS8PemKey.data(), s_PKCS8PemKey.size())) {
					stream << "OpenSSL: AES decryption failed";
					success = false;
				}
			});

			crypto::encryptBlock(crypto::Backend::MbedTLS, mbedtlsAesKey, s_PKCS8PemKey, [&] (BytesView buf) {
				mbedtlsEncoded = buf.bytes<Interface>();
			});

			crypto::decryptBlock(crypto::Backend::MbedTLS, mbedtlsAesKey, mbedtlsEncoded, [&] (BytesView buf) {
				if (buf != BytesView((const uint8_t *)s_PKCS8PemKey.data(), s_PKCS8PemKey.size())) {
					stream << "MbedTLS: AES decryption failed";
					success = false;
				}
			});

#ifdef MODULE_STAPPLER_CRYPTO_GNUTLS
			crypto::decryptBlock(crypto::Backend::GnuTLS, gnutlsAesKey, opensslEncoded, [&] (BytesView buf) {
				if (buf != BytesView((const uint8_t *)s_PKCS8PemKey.data(), s_PKCS8PemKey.size())) {
					stream << "GnuTLS: AES-cross decryption failed";
					success = false;
				}
			});

			crypto::decryptBlock(crypto::Backend::OpenSSL, gnutlsAesKey, mbedtlsEncoded, [&] (BytesView buf) {
				if (buf != BytesView((const uint8_t *)s_PKCS8PemKey.data(), s_PKCS8PemKey.size())) {
					stream << "OpenSSL: AES-cross decryption failed";
					success = false;
				}
			});

			crypto::decryptBlock(crypto::Backend::MbedTLS, mbedtlsAesKey, gnutlsEncoded, [&] (BytesView buf) {
				if (buf != BytesView((const uint8_t *)s_PKCS8PemKey.data(), s_PKCS8PemKey.size())) {
					stream << "MbedTLS: AES-cross decryption failed";
					success = false;
				}
			});

			if constexpr (crypto::SafeBlockEncoding) {
				if (mbedtlsEncoded != gnutlsEncoded) {
					return false;
				}
				if (opensslEncoded != gnutlsEncoded) {
					return false;
				}
			}
#endif

			crypto::decryptBlock(crypto::Backend::OpenSSL, opensslAesKey, mbedtlsEncoded, [&] (BytesView buf) {
				if (buf != BytesView((const uint8_t *)s_PKCS8PemKey.data(), s_PKCS8PemKey.size())) {
					stream << "OpenSSL: AES-cross decryption failed";
					success = false;
				}
			});

			crypto::decryptBlock(crypto::Backend::MbedTLS, mbedtlsAesKey, opensslEncoded, [&] (BytesView buf) {
				if (buf != BytesView((const uint8_t *)s_PKCS8PemKey.data(), s_PKCS8PemKey.size())) {
					stream << "MbedTLS: AES-cross decryption failed";
					success = false;
				}
			});

			return success;
		});

		runTest(stream, toString("pk-rsa-encrypt"), count, passed, [&] () -> bool {
			bool success = true;

			crypto::PrivateKey opensslPk(crypto::Backend::OpenSSL, s_PKCS8PemKey);
			crypto::PrivateKey mbedtlsPk(crypto::Backend::MbedTLS, s_PKCS8PemKey);

			Bytes opensslPkEncrypted;
			opensslPk.exportPublic().encrypt([&] (BytesView buf) {
				opensslPkEncrypted = buf.bytes<Interface>();;
			}, s_GostPrivKey);

			Bytes mbedtlsPkEncrypted;
			mbedtlsPk.exportPublic().encrypt([&] (BytesView buf) {
				mbedtlsPkEncrypted = buf.bytes<Interface>();;
			}, s_GostPrivKey);

#ifdef MODULE_STAPPLER_CRYPTO_GNUTLS
			crypto::PrivateKey gnutlsPk(crypto::Backend::GnuTLS, s_PKCS8PemKey);
			Bytes gnutlsPkEncrypted;
			gnutlsPk.encrypt([&] (BytesView buf) {
				gnutlsPkEncrypted = buf.bytes<Interface>();;
			}, s_GostPrivKey);
#endif

			if (!opensslPkEncrypted.empty()) {
#ifdef MODULE_STAPPLER_CRYPTO_GNUTLS
				gnutlsPk.decrypt([&] (BytesView buf) {
					if (buf != BytesView((const uint8_t *)s_GostPrivKey.data(), s_GostPrivKey.size())) {
						stream << "OpenSSL -> GnuTLS: RSA decryption failed";
						success = false;
					}
				}, opensslPkEncrypted);
#endif

				mbedtlsPk.decrypt([&] (BytesView buf) {
					if (buf != BytesView((const uint8_t *)s_GostPrivKey.data(), s_GostPrivKey.size())) {
						stream << "OpenSSL -> MbedTLS: RSA decryption failed";
						success = false;
					}
				}, opensslPkEncrypted);
			} else {
				success = false;
			}

#ifdef MODULE_STAPPLER_CRYPTO_GNUTLS
			if (!gnutlsPkEncrypted.empty()) {
				opensslPk.decrypt([&] (BytesView buf) {
					if (buf != BytesView((const uint8_t *)s_GostPrivKey.data(), s_GostPrivKey.size())) {
						stream << "GnuTLS -> OpenSSL: RSA decryption failed";
						success = false;
					}
				}, gnutlsPkEncrypted);

				mbedtlsPk.decrypt([&] (BytesView buf) {
					if (buf != BytesView((const uint8_t *)s_GostPrivKey.data(), s_GostPrivKey.size())) {
						stream << "GnuTLS -> MbedTLS: RSA decryption failed";
						success = false;
					}
				}, gnutlsPkEncrypted);
			} else {
				success = false;
			}
#endif

			if (!mbedtlsPkEncrypted.empty()) {
				opensslPk.decrypt([&] (BytesView buf) {
					if (buf != BytesView((const uint8_t *)s_GostPrivKey.data(), s_GostPrivKey.size())) {
						stream << "MbedTLS -> OpenSSL: RSA decryption failed";
						success = false;
					}
				}, mbedtlsPkEncrypted);

#ifdef MODULE_STAPPLER_CRYPTO_GNUTLS
				gnutlsPk.decrypt([&] (BytesView buf) {
					if (buf != BytesView((const uint8_t *)s_GostPrivKey.data(), s_GostPrivKey.size())) {
						stream << "MbedTLS -> GnuTLS: RSA decryption failed";
						success = false;
					}
				}, mbedtlsPkEncrypted);
#endif
			} else {
				success = false;
			}

			return success;
		});

		crypto::listBackends([&] (crypto::Backend b, StringView title, crypto::BackendFlags flags) {
			if ((flags & crypto::BackendFlags::SupportsGost3410_2012) != crypto::BackendFlags::None) {
				runTest(stream, toString(title, "-gost-sign"), count, passed, [&] () -> bool {
					return CryptoTest_gost_sign(stream, b);
				});
			}

			runTest(stream, toString(title, "-genkey"), count, passed, [&] () -> bool {
				return CryptoTest_genkey(stream, b, crypto::KeyType::GOST3410_2012_256)
						&& CryptoTest_genkey(stream, b, crypto::KeyType::GOST3410_2012_512)
						&& CryptoTest_genkey(stream, b, crypto::KeyType::RSA);
			});

			runTest(stream, toString(title, "-load"), count, passed, [&] () -> bool {
				return CryptoTest_load(stream, b);
			});

			runTest(stream, toString(title, "-pubload"), count, passed, [&] () -> bool {
				return CryptoTest_pubload(stream, b);
			});

			runTest(stream, toString(title, "-ssh"), count, passed, [&] () -> bool {
				return CryptoTest_ssh(stream, b);
			});

			runTest(stream, toString(title, "-sign-validate"), count, passed, [&] () -> bool {
				return CryptoTest_sign_validate(stream, b);
			});
		});

		_desc = stream.str();

		return count == passed;
	}
} _CryptoTest;

}

#endif
