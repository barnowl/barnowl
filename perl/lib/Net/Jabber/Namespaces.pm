##############################################################################
#
#  This library is free software; you can redistribute it and/or
#  modify it under the terms of the GNU Library General Public
#  License as published by the Free Software Foundation; either
#  version 2 of the License, or (at your option) any later version.
#
#  This library is distributed in the hope that it will be useful,
#  but WITHOUT ANY WARRANTY; without even the implied warranty of
#  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
#  Library General Public License for more details.
#
#  You should have received a copy of the GNU Library General Public
#  License along with this library; if not, write to the
#  Free Software Foundation, Inc., 59 Temple Place - Suite 330,
#  Boston, MA  02111-1307, USA.
#
#  Copyright (C) 1998-2004 Jabber Software Foundation http://jabber.org/
#
##############################################################################

package Net::Jabber::Namespaces;

=head1 NAME

Net::Jabber::Namespaces

=head1 SYNOPSIS

  Net::Jabber::Namespaces is a pure documentation
  module.  It provides no code for execution, just
  documentation on how the Net::Jabber modules handle
  namespaces.

=head1 DESCRIPTION

  Net::Jabber::Namespaces is fully documented by Net::XMPP::Namesapces.

=head1 AUTHOR

Ryan Eatmon

=head1 COPYRIGHT

This module is free software; you can redistribute it and/or modify
it under the same terms as Perl itself.

=cut

use Net::XMPP::Namespaces;

$Net::XMPP::Namespaces::SKIPNS{'__netjabber__'} = 1;

#-----------------------------------------------------------------------------
# jabber:iq:agent
#-----------------------------------------------------------------------------
{
    &add_ns(ns    => 'jabber:iq:agent',
            tag   => 'query',
            xpath => {
                      Agents      => {
                                      type => 'flag',
                                      path => 'agents',
                                     },
                      Description => { path => 'description/text()' },
                      JID         => {
                                      type => 'jid',
                                      path => '@jid',
                                     },
                      Name        => { path => 'name/text()' },
                      GroupChat   => {
                                      type => 'flag',
                                      path => 'groupchat',
                                     },
                      Register    => {
                                      type => 'flag',
                                      path => 'register',
                                     },
                      Search      => {
                                      type => 'flag',
                                      path => 'search',
                                     },
                      Service     => { path => 'service/text()' },
                      Transport   => { path => 'transport/text()' },
                      URL         => { path => 'url/text()' },
                      Agent       => { type => 'master' },
                     },
            docs  => {
                      module     => 'Net::Jabber',
                      deprecated => 1,
                     },

           );
}

#-----------------------------------------------------------------------------
# jabber:iq:agents
#-----------------------------------------------------------------------------
{
    &add_ns(ns    => 'jabber:iq:agents',
            tag   => 'query',
            xpath => {
                      Agent  => {
                                 type  => 'child',
                                 path  => 'agent',
                                 child => {
                                           ns         => 'jabber:iq:agent',
                                           skip_xmlns => 1,
                                          },
                                 calls => [ 'Add' ],
                                },
                      Agents => {
                                 type  => 'child',
                                 path  => 'agent',
                                 child => { ns => 'jabber:iq:agent' },
                                },
                     },
            docs  => {
                      module     => 'Net::Jabber',
                      deprecated => 1,
                     },

           );
}

#-----------------------------------------------------------------------------
# jabber:iq:autoupdate
#-----------------------------------------------------------------------------
{
    &add_ns(ns    => 'jabber:iq:autoupdate',
            tag   => 'query',
            xpath => {
                      Beta     => {
                                   type  => 'child',
                                   path  => 'beta',
                                   child => { ns => '__netjabber__:iq:autoupdate:release' },
                                   calls => [ 'Add' ],
                                  },
                      Dev      => {
                                   type  => 'child',
                                   path  => 'dev',
                                   child => { ns => '__netjabber__:iq:autoupdate:release' },
                                   calls => [ 'Add' ],
                                  },
                      Release  => {
                                   type  => 'child',
                                   path  => 'release',
                                   child => { ns => '__netjabber__:iq:autoupdate:release' },
                                   calls => [ 'Add' ],
                                  },
                      Releases => {
                                   type  => 'child',
                                   path  => '*',
                                   child => { ns => '__netjabber__:iq:autoupdate:release' },
                                  },
                     },
            docs  => {
                      module => 'Net::Jabber',
                     },

           );
}

#-----------------------------------------------------------------------------
# __netjabber__:iq:autoupdate:release
#-----------------------------------------------------------------------------
{
    &add_ns(ns    => '__netjabber__:iq:autoupdate:release',
            xpath => {
                      Desc     => { path => 'desc/text()' },
                      Priority => { path => '@priority' },
                      URL      => { path => 'url/text()' },
                      Version  => { path => 'version/text()' },
                      Release  => { type => 'master' },
                     },
            docs  => {
                      module => 'Net::Jabber',
                      name   => 'jabber:iq:autoupdate - release objects',
                     },
           );
}

#-----------------------------------------------------------------------------
# jabber:iq:browse
#-----------------------------------------------------------------------------
{
    &add_ns(ns    => 'jabber:iq:browse',
            tag   => 'item',
            xpath => {
                      Category => { path => '@category' },
                      Item     => {
                                   type  => 'child',
                                   path  => '*[name() != "ns"]',
                                   child => { ns           => '__netjabber__:iq:browse:item',
                                              specify_name => 1,
                                              tag          => 'item',
                                            },
                                   calls => [ 'Add' ],
                                  },
                      Items    => {
                                   type  => 'child',
                                   path  => '*[name() != "ns"]',
                                   child => { ns => '__netjabber__:iq:browse:item' },
                                  },
                      JID      => {
                                   type => 'jid',
                                   path => '@jid',
                                  },
                      Name     => { path => '@name' },
                      NS       => {
                                   type => 'array',
                                   path => 'ns/text()',
                                  },
                      Type     => { path => '@type' },
                      Browse   => { type => 'master' }
                     },
            docs  => {
                      module     => 'Net::Jabber',
                      deprecated => 1,
                     },

           );
}

#-----------------------------------------------------------------------------
# __netjabber__:iq:browse:item
#-----------------------------------------------------------------------------
{
    &add_ns(ns    => '__netjabber__:iq:browse:item',
            tag   => 'item',
            xpath => {
                      Category => { path => '@category' },
                      Item     => {
                                   type  => 'child',
                                   path  => '*[name() != "ns"]',
                                   child => { ns           => '__netjabber__:iq:browse:item',
                                              specify_name => 1,
                                              tag          => 'item',
                                            },
                                   calls => [ 'Add' ],
                                  },
                      Items    => {
                                   type  => 'child',
                                   path  => '*[name() != "ns"]',
                                   child => { ns => '__netjabber__:iq:browse:item' },
                                  },
                      JID      => {
                                   type => 'jid',
                                   path => '@jid',
                                  },
                      Name     => { path => '@name' },
                      NS       => {
                                   type => 'array',
                                   path => 'ns/text()',
                                  },
                      Type     => { path => '@type' },
                      Browse   => { type => 'master' }
                     },
            docs  => {
                      module     => 'Net::Jabber',
                      name       => 'jabber:iq:browse - item objects',
                      deprecated => 1,
                     },

           );
}

#-----------------------------------------------------------------------------
# jabber:iq:conference
#-----------------------------------------------------------------------------
{
    &add_ns(ns    => 'jabber:iq:conference',
            tag   => 'query',
            xpath => {
                      ID         => { path => 'id/text()' },
                      Name       => { path => 'name/text()' },
                      Nick       => { path => 'nick/text()' },
                      Privacy    => {
                                     type => 'flag',
                                     path => 'privacy',
                                    },
                      Secret     => { path => 'secret/text()' },
                      Conference => { type => 'master' },
                     },
            docs  => {
                      module => 'Net::Jabber',
                     },

           );
}

#-----------------------------------------------------------------------------
# jabber:iq:filter
#-----------------------------------------------------------------------------
{
    &add_ns(ns    => 'jabber:iq:filter',
            tag   => 'query',
            xpath => {
                      Rule  => {
                                type  => 'child',
                                path  => 'rule',
                                child => { ns => '__netjabber__:iq:filter:rule' },
                                calls => [ 'Add' ],
                               },
                      Rules => {
                                type  => 'child',
                                path  => 'rule',
                                child => { ns => '__netjabber__:iq:filter:rule' },
                               },
                     },
            docs  => {
                      module     => 'Net::Jabber',
                      deprecated => 1,
                     },

           );
}

#-----------------------------------------------------------------------------
# __netjabber__:iq:filter:rule
#-----------------------------------------------------------------------------
{
    &add_ns(ns     => '__netjabber__:iq:filter:rule',
            xpath  => {
                       Body        => { path => 'body/text()' },
                       Continued   => { path => 'continued/text()' },
                       Drop        => { path => 'drop/text()' },
                       Edit        => { path => 'edit/text()' },
                       Error       => { path => 'error/text()' },
                       From        => { path => 'from/text()' },
                       Offline     => { path => 'offline/text()' },
                       Reply       => { path => 'reply/text()' },
                       Resource    => { path => 'resource/text()' },
                       Show        => { path => 'show/text()' },
                       Size        => { path => 'size/text()' },
                       Subject     => { path => 'subject/text()' },
                       Time        => { path => 'time/text()' },
                       Type        => { path => 'type/text()' },
                       Unavailable => { path => 'unavailable/text()' },
                       Rule        => { type => 'master' },
                      },
            docs  => {
                      module     => 'Net::Jabber',
                      name       => 'jabber:iq:filter - rule objects',
                      deprecated => 1,
                     },

           );
}

#-----------------------------------------------------------------------------
# jabber:iq:gateway
#-----------------------------------------------------------------------------
{
    &add_ns(ns    => 'jabber:iq:gateway',
            tag   => 'query',
            xpath => {
                      Desc    => { path => 'desc/text()' },
                      JID     => {
                                  type => 'jid',
                                  path => 'jid/text()',
                                 },
                      Prompt  => { path => 'prompt/text()' },
                      Gateway => { type => 'master' },
                     },
            docs  => {
                      module => 'Net::Jabber',
                     },

           );
}

#-----------------------------------------------------------------------------
# jabber:iq:last
#-----------------------------------------------------------------------------
{
    &add_ns(ns    => 'jabber:iq:last',
            tag   => 'query',
            xpath => {
                      Message => { path => 'text()' },
                      Seconds => { path => '@seconds' },
                      Last    => { type => 'master' },
                     },
            docs  => {
                      module => 'Net::Jabber',
                     },

           );
}

#-----------------------------------------------------------------------------
# jabber:iq:oob
#-----------------------------------------------------------------------------
{
    &add_ns(ns    => 'jabber:iq:oob',
            tag   => 'query',
            xpath => {
                      Desc => { path => 'desc/text()' },
                      URL  => { path => 'url/text()' },
                      Oob  => { type => 'master' },
                     },
            docs  => {
                      module => 'Net::Jabber',
                     },

           );
}

#-----------------------------------------------------------------------------
# jabber:iq:pass
#-----------------------------------------------------------------------------
{
    &add_ns(ns    => 'jabber:iq:pass',
            tag   => 'query',
            xpath => {
                      Client     => { path => 'client/text()' },
                      ClientPort => { path => 'client/@port' },
                      Close      => {
                                     type => 'flag',
                                     path => 'close',
                                    },
                      Expire     => { path => 'expire/text()' },
                      OneShot    => {
                                     type => 'flag',
                                     path => 'oneshot',
                                    },
                      Proxy      => { path => 'proxy/text()' },
                      ProxyPort  => { path => 'proxy/@port' },
                      Server     => { path => 'server/text()' },
                      ServerPort => { path => 'server/@port' },
                      Pass       => { type => 'master' },
                     },
            docs  => {
                      module => 'Net::Jabber',
                     },

           );
}

#-----------------------------------------------------------------------------
# jabber:iq:rpc
#-----------------------------------------------------------------------------
{
    &add_ns(ns    => 'jabber:iq:rpc',
            tag   => 'query',
            xpath => {
                      MethodCall     => {
                                         type  => 'child',
                                         path  => 'methodCall',
                                         child => { ns => '__netjabber__:iq:rpc:methodCall' },
                                         calls => [ 'Get', 'Defined', 'Add', 'Remove' ],
                                        },
                      MethodResponse => {
                                         type  => 'child',
                                         path  => 'methodResponse',
                                         child => { ns => '__netjabber__:iq:rpc:methodResponse' },
                                         calls => [ 'Get', 'Defined', 'Add', 'Remove' ],
                                        },
                     },
            docs  => {
                      module => 'Net::Jabber',
                     },

           );
}

#-----------------------------------------------------------------------------
# __netjabber__:iq:rpc:methodCall
#-----------------------------------------------------------------------------
{
    &add_ns(ns    => '__netjabber__:iq:rpc:methodCall',
            xpath => {
                      MethodName => { path => 'methodName/text()' },
                      Params     => {
                                     type  => 'child',
                                     path  => 'params',
                                     child => { ns => '__netjabber__:iq:rpc:params' },
                                     calls => [ 'Get', 'Defined', 'Add', 'Remove' ],
                                     },
                      MethodCall => { type => 'master' },
                     },
            docs  => {
                      module => 'Net::Jabber',
                      name   => 'jabber:iq:rpc - methodCall objects',
                     },

           );
}
       
#-----------------------------------------------------------------------------
# __netjabber__:iq:rpc:methodResponse
#-----------------------------------------------------------------------------
{
    &add_ns(ns    => '__netjabber__:iq:rpc:methodResponse',
            xpath => {
                      Fault  => {
                                 type  => 'child',
                                 path  => 'fault',
                                 child => { ns => '__netjabber__:iq:rpc:fault' },
                                 calls => [ 'Get', 'Defined', 'Add', 'Remove' ],
                                },
                      Params => {
                                 type  => 'child',
                                 path  => 'params',
                                 child => { ns => '__netjabber__:iq:rpc:params' },
                                 calls => [ 'Get', 'Defined', 'Add', 'Remove' ],
                                },
                     },
            docs  => {
                      module => 'Net::Jabber',
                      name   => 'jabber:iq:rpc - methodResponse objects',
                     },

           );
}
       
#-----------------------------------------------------------------------------
# __netjabber__:iq:rpc:fault
#-----------------------------------------------------------------------------
{
    &add_ns(ns    =>'__netjabber__:iq:rpc:fault',
            xpath => {
                      Value => {
                                type  => 'child',
                                path  => 'value',
                                child => { ns => '__netjabber__:iq:rpc:value' },
                                calls => [ 'Get', 'Defined', 'Add', 'Remove' ],
                               },
                     },
            docs  => {
                      module => 'Net::Jabber',
                      name   => 'jabber:iq:rpc - fault objects',
                     },

           );
}
       
#-----------------------------------------------------------------------------
# __netjabber__:iq:rpc:params
#-----------------------------------------------------------------------------
{
    &add_ns(ns    => '__netjabber__:iq:rpc:params',
            xpath => {
                      Param  => {
                                 type  => 'child',
                                 path  => 'param',
                                 child => { ns => '__netjabber__:iq:rpc:param' },
                                 calls => [ 'Add' ],
                                },
                      Params => {
                                 type  => 'child',
                                 path  => 'param',
                                 child => { ns => '__netjabber__:iq:rpc:param' },
                                },
                     },
            docs  => {
                      module => 'Net::Jabber',
                      name   => 'jabber:iq:rpc - params objects',
                     },

           );
}

#-----------------------------------------------------------------------------
# __netjabber__:iq:rpc:param
#-----------------------------------------------------------------------------
{
    &add_ns(ns    =>'__netjabber__:iq:rpc:param',
            xpath => {
                      Value => {
                                type  => 'child',
                                path  => 'value',
                                child => { ns => '__netjabber__:iq:rpc:value' },
                                calls => [ 'Get', 'Defined', 'Add', 'Remove' ],
                               },
                     },
            docs  => {
                      module => 'Net::Jabber',
                      name   => 'jabber:iq:rpc - param objects',
                     },
           );
}
       
#-----------------------------------------------------------------------------
# __netjabber__:iq:rpc:value
#-----------------------------------------------------------------------------
{
    &add_ns(ns    => '__netjabber__:iq:rpc:value',
            xpath => {
                      Array    => {
                                   type  => 'child',
                                   path  => 'array',
                                   child => { ns => '__netjabber__:iq:rpc:array' },
                                   calls => [ 'Get', 'Defined', 'Add', 'Remove' ],
                                  },
                      Base64   => { path => 'base64/text()' },
                      Boolean  => { path => 'boolean/text()' },
                      DateTime => { path => 'dateTime.iso8601/text()' },
                      Double   => { path => 'double/text()' },
                      I4       => { path => 'i4/text()' },
                      Int      => { path => 'int/text()' },
                      String   => { path => 'string/text()' },
                      Struct   => {
                                   type  => 'child',
                                   path  => 'struct',
                                   child => { ns => '__netjabber__:iq:rpc:struct' },
                                   calls => [ 'Get', 'Defined', 'Add', 'Remove' ],
                                  },
                      Value    => { path => 'value/text()' },
                      RPCValue => { type => 'master' },
                     },
            docs  => {
                      module => 'Net::Jabber',
                      name   => 'jabber:iq:rpc - value objects',
                     },
           );
}

#-----------------------------------------------------------------------------
# __netjabber__:iq:rpc:struct
#-----------------------------------------------------------------------------
{
    &add_ns(ns    => '__netjabber__:iq:rpc:struct',
            xpath => {
                      Member   => {
                                  type  => 'child',
                                  path  => 'member',
                                  child => { ns => '__netjabber__:iq:rpc:struct:member' },
                                  calls => [ 'Add' ],
                                 },
                      Members => {
                                  type  => 'child',
                                  path  => 'member',
                                  child => { ns => '__netjabber__:iq:rpc:struct:member' },
                                 },
                     },
            docs  => {
                      module => 'Net::Jabber',
                      name   => 'jabber:iq:rpc - struct objects',
                     },

           );
}

#-----------------------------------------------------------------------------
# __netjabber__:iq:rpc:struct:member
#-----------------------------------------------------------------------------
{
    &add_ns(ns    =>'__netjabber__:iq:rpc:struct:member',
            xpath => {
                      Name   => { path => 'name/text()' },
                      Value  => {
                                 type  => 'child',
                                 path  => 'value',
                                 child => { ns => '__netjabber__:iq:rpc:value' },
                                 calls => [ 'Get', 'Defined', 'Add', 'Remove' ],
                                },
                      Member => { type => 'master' },
                     },
            docs  => {
                      module => 'Net::Jabber',
                      name   => 'jabber:iq:rpc - member objects',
                     },

          );
}

#-----------------------------------------------------------------------------
# __netjabber__:iq:rpc:array
#-----------------------------------------------------------------------------
{
    &add_ns(ns    => '__netjabber__:iq:rpc:array',
            xpath => {
                      Data  => {
                                type  => 'child',
                                path  => 'data',
                                child => { ns => '__netjabber__:iq:rpc:array:data' },
                                calls => [ 'Add' ],
                               },
                      Datas => {
                                type  => 'child',
                                path  => 'data',
                                child => { ns => '__netjabber__:iq:rpc:array:data' },
                               },
                     },
            docs  => {
                      module => 'Net::Jabber',
                      name   => 'jabber:iq:rpc - array objects',
                     },

           );
}

#-----------------------------------------------------------------------------
# __netjabber__:iq:rpc:array:data
#-----------------------------------------------------------------------------
{
    &add_ns(ns    =>'__netjabber__:iq:rpc:array:data',
            xpath => {
                      Value => {
                                type  => 'child',
                                path  => 'value',
                                child => { ns => '__netjabber__:iq:rpc:value' },
                                calls => [ 'Get', 'Defined', 'Add', 'Remove' ],
                               },
                     },
            docs  => {
                      module => 'Net::Jabber',
                      name   => 'jabber:iq:rpc - data objects',
                     },

           );
}

#-----------------------------------------------------------------------------
# jabber:iq:search
#-----------------------------------------------------------------------------
{
    &add_ns(ns    => 'jabber:iq:search',
            tag   => 'query',
            xpath => {
                      Email        => { path => 'email/text()' },
                      Family       => { path => 'family/text()' },
                      First        => { path => 'first/text()' },
                      Given        => { path => 'given/text()' },
                      Instructions => { path => 'instructions/text()' },
                      Item         => {
                                       type  => 'child',
                                       path  => 'item',
                                       child => { ns => '__netjabber__:iq:search:item' },
                                       calls => [ 'Add' ],
                                      },
                      Items        => {
                                       type  => 'child',
                                       path  => 'item',
                                       child => { ns => '__netjabber__:iq:search:item', },
                                      },
                      Key          => { path => 'key/text()' },
                      Last         => { path => 'last/text()' },
                      Name         => { path => 'name/text()' },
                      Nick         => { path => 'nick/text()' },
                      Truncated    => {
                                       type => 'flag',
                                       path => 'truncated',
                                      },
                      Search       => { type => 'master' },
                     },
            docs  => {
                      module => 'Net::Jabber',
                     },
           );
}

#-----------------------------------------------------------------------------
# __netjabber__:iq:search:item
#-----------------------------------------------------------------------------
{
    &add_ns(ns    => '__netjabber__:iq:search:item',
            xpath => {
                      Email  => { path => 'email/text()' },
                      Family => { path => 'family/text()' },
                      First  => { path => 'first/text()' },
                      Given  => { path => 'given/text()' },
                      JID    => {
                                 type => 'jid',
                                 path => '@jid',
                                },
                      Key    => { path => 'key/text()' },
                      Last   => { path => 'last/text()' },
                      Name   => { path => 'name/text()' },
                      Nick   => { path => 'nick/text()' },
                      Item   => { type => 'master' },
                     },
            docs  => {
                      module => 'Net::Jabber',
                      name   => 'jabber:iq:search - item objects',
                     },
           );
}

#-----------------------------------------------------------------------------
# jabber:iq:time
#-----------------------------------------------------------------------------
{
    &add_ns(ns    => 'jabber:iq:time',
            tag   => 'query',
            xpath => {
                      Display => {
                                  type  => ['special','time-display'],
                                  path  => 'display/text()',
                                 },
                      TZ      => {
                                  type  => ['special','time-tz'],
                                  path  => 'tz/text()',
                                 },
                      UTC     => {
                                  type  => ['special','time-utc'],
                                  path  => 'utc/text()',
                                 },
                      Time    => { type => [ 'master', 'all' ] }
                      },
            docs  => {
                      module => 'Net::Jabber',
                     },
           );
}
                                   
#-----------------------------------------------------------------------------
# jabber:iq:version
#-----------------------------------------------------------------------------
{
    &add_ns(ns    => 'jabber:iq:version',
            tag   => 'query',
            xpath => {
                      Name    => { path => 'name/text()' },
                      OS      => {
                                  type => [ 'special', 'version-os' ],
                                  path => 'os/text()',
                                 },
                      Ver     => {
                                  type => [ 'special' ,'version-version' ],
                                  path => 'version/text()',
                                 },
                      Version => { type => [ 'master', 'all' ] }
                     },
            docs  => {
                      module => 'Net::Jabber',
                     },
           );
}

#-----------------------------------------------------------------------------
# jabber:x:autoupdate
#-----------------------------------------------------------------------------
{
    &add_ns(ns    => 'jabber:x:autoupdate',
            tag   => 'x',
            xpath => {
                      JID => {
                              type => 'jid',
                              path => '@jid',
                             },
                      Autoupdate => { type => 'master' },
                     },
            docs  => {
                      module => 'Net::Jabber',
                     },
           );
}
                  
#-----------------------------------------------------------------------------
# jabber:x:conference
#-----------------------------------------------------------------------------
{
    &add_ns(ns    => 'jabber:x:conference',
            tag   => 'x',
            xpath => {
                      JID => {
                              type => 'jid',
                              path => '@jid',
                             },
                      Conference => { type => 'master' },
                     },
            docs  => {
                      module => 'Net::Jabber',
                     },
           );
}

#-----------------------------------------------------------------------------
# jabber:x:data
#-----------------------------------------------------------------------------
{
    &add_ns(ns    => 'jabber:x:data',
            tag   => 'x',
            xpath => {
                      Field        => {
                                       type  => 'child',
                                       path  => 'field',
                                       child => { ns => '__netjabber__:x:data:field' },
                                       calls => [ 'Add' ],
                                      },
                      Fields       => {
                                       type  => 'child',
                                       path  => 'field',
                                       child => { ns => '__netjabber__:x:data:field', },
                                      },
                      Form         => { path => '@form' },
                      Instructions => { path => 'instructions/text()' },
                      Item         => {
                                       type  => 'child',
                                       path  => 'item',
                                       child => { ns => '__netjabber__:x:data:item' },
                                       calls => [ 'Add' ],
                                      },
                      Items        => {
                                       type  => 'child',
                                       path  => 'item',
                                       child => { ns => '__netjabber__:x:data:item', },
                                      },
                      Reported     => {
                                       type  => 'child',
                                       path  => 'reported',
                                       child => { ns => '__netjabber__:x:data:reported' },
                                       calls => [ 'Get', 'Defined', 'Add', 'Remove' ],
                                      },
                      Title        => { path => 'title/text()' },
                      Type         => { path => '@type' },
                      Data         => { type => 'master' },
                     },
            docs  => {
                      module => 'Net::Jabber',
                     },
           );
}

#-----------------------------------------------------------------------------
# __netjabber__:x:data:field
#-----------------------------------------------------------------------------
{
    &add_ns(ns    => '__netjabber__:x:data:field',
            xpath => {
                      Desc     => { path => 'desc/text()' },
                      Label    => { path => '@label' },
                      Option   => {
                                   type  => 'child',
                                   path  => 'option',
                                   child => { ns => '__netjabber__:x:data:field:option' },
                                   calls => [ 'Add' ],
                                  },
                      Options  => {
                                   type  => 'child',
                                   path  => 'option',
                                   child => { ns => '__netjabber__:x:data:field:option', },
                                  },
                      Required => {
                                   type => 'flag',
                                   path => 'required',
                                  },
                      Type     => { path => '@type' },
                      Value    => {
                                   type => 'array',
                                   path => 'value/text()',
                                  },
                      Var      => { path => '@var' },
                      Field  => { type => 'master' },
                     },
            docs  => {
                      module => 'Net::Jabber',
                      name   => 'jabber:x:data - field objects',
                     },
           );
}

#-----------------------------------------------------------------------------
# __netjabber__:x:data:field:option
#-----------------------------------------------------------------------------
{
    &add_ns(ns    => '__netjabber__:x:data:field:option',
            xpath => {
                      Label  => { path => '@label' },
                      Value  => { path => 'value/text()' },
                      Option => { type => 'master' },
                     },
            docs  => {
                      module => 'Net::Jabber',
                      name   => 'jabber:x:data - option objects',
                     },
           );
}
       
#-----------------------------------------------------------------------------
# __netjabber__:x:data:item
#-----------------------------------------------------------------------------
{
    &add_ns(ns    => '__netjabber__:x:data:item',
            xpath => {
                      Field        => {
                                       type  => 'child',
                                       path  => 'field',
                                       child => { ns => '__netjabber__:x:data:field' },
                                       calls => [ 'Add' ],
                                      },
                      Fields       => {
                                       type  => 'child',
                                       path  => 'field',
                                       child => { ns => '__netjabber__:x:data:field', },
                                      },
                      Item         => { type => 'master' },
                     },
            docs  => {
                      module => 'Net::Jabber',
                      name   => 'jabber:x:data - item objects',
                     },
           );
}

#-----------------------------------------------------------------------------
# __netjabber__:x:data:reported
#-----------------------------------------------------------------------------
{
    &add_ns(ns    => '__netjabber__:x:data:reported',
            xpath => {
                      Field        => {
                                       type  => 'child',
                                       path  => 'field',
                                       child => { ns => '__netjabber__:x:data:field' },
                                       calls => [ 'Add' ],
                                      },
                      Fields       => {
                                       type  => 'child',
                                       path  => 'field',
                                       child => { ns => '__netjabber__:x:data:field', },
                                      },
                      Reported     => { type => 'master' },
                     },
            docs  => {
                      module => 'Net::Jabber',
                      name   => 'jabber:x:data - reported objects',
                     },
           );
}

#-----------------------------------------------------------------------------
# jabber:x:delay
#-----------------------------------------------------------------------------
{
    &add_ns(ns    => 'jabber:x:delay',
            tag   => 'x',
            xpath => {
                      From    => {
                                  type => 'jid',
                                  path => '@from',
                                 },
                      Message => { path => 'text()' },
                      Stamp   => {
                                  type => 'timestamp',
                                  path => '@stamp',
                                 },
                      Delay   => { type => 'master' },
                     },
            docs  => {
                      module => 'Net::Jabber',
                     },
           );
}

#-----------------------------------------------------------------------------
# jabber:x:encrypted
#-----------------------------------------------------------------------------
{
    &add_ns(ns    => 'jabber:x:encrypted',
            tag   => 'x',
            xpath => {
                      Message   => { path => 'text()' },
                      Encrypted => { type => 'master' },
                     },
            docs  => {
                      module => 'Net::Jabber',
                     },
           );
}
       
#-----------------------------------------------------------------------------
# jabber:x:event
#-----------------------------------------------------------------------------
{
    &add_ns(ns    => 'jabber:x:event',
            tag   => 'x',
            xpath => {
                      Composing => {
                                    type => 'flag',
                                    path => 'composing',
                                   },
                      Delivered => {
                                    type => 'flag',
                                    path => 'delivered',
                                   },
                      Displayed => {
                                    type => 'flag',
                                    path => 'displayed',
                                   },
                      ID        => { path => 'id/text()' },
                      Offline   => {
                                    type => 'flag',
                                    path => 'offline',
                                   },
                      Event     => { type => 'master' },
                     },
            docs  => {
                      module => 'Net::Jabber',
                     },
           );
}

#-----------------------------------------------------------------------------
# jabber:x:expire
#-----------------------------------------------------------------------------
{
    &add_ns(ns    => 'jabber:x:expire',
            tag   => 'x',
            xpath => {
                      Seconds => { path => '@seconds' },
                      Expire  => { type => 'master' },
                     },
            docs  => {
                      module => 'Net::Jabber',
                     },
           );
}

#-----------------------------------------------------------------------------
# jabber:x:oob
#-----------------------------------------------------------------------------
{
    &add_ns(ns    => 'jabber:x:oob',
            tag   => 'x',
            xpath => {
                      Desc => { path => 'desc/text()' },
                      URL  => { path => 'url/text()' },
                      Oob  => { type => 'master' },
                     },
            docs  => {
                      module => 'Net::Jabber',
                     },
           );
}

#-----------------------------------------------------------------------------
# jabber:x:roster
#-----------------------------------------------------------------------------
{
    &add_ns(ns    => 'jabber:x:roster',
            tag   => 'x',
            xpath => {
                      Item  => {
                                type  => 'child',
                                path  => 'item',
                                child => { ns => '__netjabber__:x:roster:item' },
                                calls => [ 'Add' ],
                               },
                      Items => {
                                type  => 'child',
                                path  => 'item',
                                child => { ns => '__netjabber__:x:roster:item', },
                               },
                      Roster => { type => 'master' },
                     },
            docs  => {
                      module => 'Net::Jabber',
                     },
           );
}
       
#-----------------------------------------------------------------------------
# __netjabber__:x:roster:item
#-----------------------------------------------------------------------------
{
    &add_ns(ns    => '__netjabber__:x:roster:item',
            xpath => {
                      Ask          => { path => '@ask' },
                      Group        => {
                                       type => 'array',
                                       path => 'group/text()',
                                      },
                      JID          => {
                                       type => 'jid',
                                       path => '@jid',
                                      },
                      Name         => { path => '@name' },
                      Subscription => { path => '@subscription' },
                      Item         => { type => 'master' },
                     },
            docs  => {
                      module => 'Net::Jabber',
                      name   => 'jabber:x:roster - item objects',
                     },
           );
}

#-----------------------------------------------------------------------------
# jabber:x:signed
#-----------------------------------------------------------------------------
{
    &add_ns(ns    => 'jabber:x:signed',
            tag   => 'x',
            xpath => {
                      Signature => { path => 'text()' },
                      Signed    => { type => 'master' },
                     },
            docs  => {
                      module => 'Net::Jabber',
                     },
           );
}

#-----------------------------------------------------------------------------
# http://jabber.org/protocol/bytestreams
#-----------------------------------------------------------------------------
{
    &add_ns(ns    => 'http://jabber.org/protocol/bytestreams',
            tag   => 'query',
            xpath => {
                      Activate => { path => 'activate/text()' },
                      SID               => { path => '@sid' },
                      StreamHost        => {
                                            type  => 'child',
                                            path  => 'streamhost',
                                            child => { ns => '__netjabber__:iq:bytestreams:streamhost' },
                                            calls => [ 'Add' ],
                                           },
                      StreamHosts       => {
                                            type  => 'child',
                                            path  => 'streamhost',
                                            child => { ns  => '__netjabber__:iq:bytestreams:streamhost', },
                                           },
                      StreamHostUsedJID => {
                                            type => 'jid',
                                            path => 'streamhost-used/@jid',
                                           },
                      ByteStreams       => { type => 'master' },
                     },
            docs  => {
                      module => 'Net::Jabber',
                     },
           );
}

#-----------------------------------------------------------------------------
# __netjabber__:iq:bytestreams:streamhost
#-----------------------------------------------------------------------------
{
    &add_ns(ns    => '__netjabber__:iq:bytestreams:streamhost',
            xpath => {
                      Host       => { path => '@host' },
                      JID        => {
                                     type => 'jid',
                                     path => '@jid',
                                    },
                      Port       => { path => '@port' },
                      ZeroConf   => { path => '@zeroconf' },
                      StreamHost => { type => 'master' },
                     },
            docs  => {
                      module => 'Net::Jabber',
                      name   => 'http://jabber.org/protocol/bytestreams - streamhost objects',
                     },
           );
}

#-----------------------------------------------------------------------------
# http://jabber.org/protocol/commands
#-----------------------------------------------------------------------------
{
    &add_ns(ns    => 'http://jabber.org/protocol/commands',
            tag   => 'command',
            xpath => {
                      Action    => { path => '@action' },
                      Node      => { path => '@node' },
                      Note      => {
                                    type  => 'child',
                                    path  => 'note',
                                    child => { ns  => '__netjabber__:iq:commands:note' },
                                    calls => [ 'Add' ],
                                   },
                      Notes     => {
                                    type  => 'child',
                                    path  => 'note',
                                    child => { ns => '__netjabber__:iq:commands:note', },
                                   },
                      SessionID => { path => '@sessionid' },
                      Status    => { path => '@status' },
                      Command   => { type => 'master' },
                     },
            docs  => {
                      module => 'Net::Jabber',
                     },
           );
}

# xxx xml:lang

#-----------------------------------------------------------------------------
# __netjabber__:iq:commands:note
#-----------------------------------------------------------------------------
{
    &add_ns(ns    => '__netjabber__:iq:commands:note',
            xpath => {
                      Type    => { path => '@type' },
                      Message => { path => 'text()' },
                      Note    => { type => 'master' },
                     },
            docs  => {
                      module => 'Net::Jabber',
                      name   => 'http://jabber.org/protocol/commands - note objects',
                     },
           );
}

#-----------------------------------------------------------------------------
# http://jabber.org/protocol/disco#info
#-----------------------------------------------------------------------------
{
    &add_ns(ns    => 'http://jabber.org/protocol/disco#info',
            tag   => 'query',
            xpath => {
                      Feature    => {
                                     type  => 'child',
                                     path  => 'feature',
                                     child => { ns => '__netjabber__:iq:disco:info:feature' },
                                     calls => [ 'Add' ],
                                    },
                      Features   => {
                                     type  => 'child',
                                     path  => 'feature',
                                     child => { ns => '__netjabber__:iq:disco:info:feature' },
                                    },
                      Identity   => {
                                     type  => 'child',
                                     path  => 'identity',
                                     child => { ns => '__netjabber__:iq:disco:info:identity' },
                                     calls => [ 'Add' ],
                                    },
                      Identities => {
                                     type  => 'child',
                                     path  => 'identity',
                                     child => { ns => '__netjabber__:iq:disco:info:identity' },
                                    },
                      Node       => { path => '@node' },
                      DiscoInfo  => { type => 'master' },
                     },
            docs  => {
                      module => 'Net::Jabber',
                     },
           );
}

#-----------------------------------------------------------------------------
# __netjabber__:iq:disco:info:feature
#-----------------------------------------------------------------------------
{
    &add_ns(ns    => '__netjabber__:iq:disco:info:feature',
            xpath => {
                      Var     => { path => '@var' },
                      Feature => { type => 'master' },
                     },
            docs  => {
                      module => 'Net::Jabber',
                      name   => 'http://jabber.org/protocol/disco#info - feature objects',
                     },
           );
}

#-----------------------------------------------------------------------------
# __netjabber__:iq:disco:info:identity
#-----------------------------------------------------------------------------
{
    &add_ns(ns    => '__netjabber__:iq:disco:info:identity',
            xpath => {
                      Category => { path => '@category' },
                      Name     => { path => '@name' },
                      Type     => { path => '@type' },
                      Identity => { type => 'master' },
                     },
            docs  => {
                      module => 'Net::Jabber',
                      name   => 'http://jabber.org/protocol/disco#info - identity objects',
                     },
           );
}

#-----------------------------------------------------------------------------
# http://jabber.org/protocol/disco#items
#-----------------------------------------------------------------------------
{
    &add_ns(ns    => 'http://jabber.org/protocol/disco#items',
            tag   => 'query',
            xpath => {
                      Item       => {
                                     type  => 'child',
                                     path  => 'item',
                                     child => { ns => '__netjabber__:iq:disco:items:item' },
                                     calls => [ 'Add' ],
                                    },
                      Items      => {
                                     type  => 'child',
                                     path  => 'item',
                                     child => { ns => '__netjabber__:iq:disco:items:item' },
                                    },
                      Node       => { path => '@node' },
                      DiscoItems => { type => 'master' },
                     },
            docs  => {
                      module => 'Net::Jabber',
                     },
           );
}
       
#-----------------------------------------------------------------------------
# __netjabber__:iq:disco:items:item
#-----------------------------------------------------------------------------
{
    &add_ns(ns    => '__netjabber__:iq:disco:items:item',
            xpath => {
                      Action => { path => '@action' },
                      JID    => {
                                 type => 'jid',
                                 path => '@jid',
                                },
                      Name   => { path => '@name' },
                      Node   => { path => '@node' },
                      Item   => { type => 'master' },
                     },
            docs  => {
                      module => 'Net::Jabber',
                      name   => 'http://jabber.org/protocol/disco#items - item objects',
                     },
           );
}

#-----------------------------------------------------------------------------
# http://jabber.org/protocol/feature-neg
#-----------------------------------------------------------------------------
{
    &add_ns(ns    => 'http://jabber.org/protocol/feature-neg',
            tag   => 'feature',
            xpath => {
                      FeatureNeg => { type => 'master' },
                     },
            docs  => {
                      module => 'Net::Jabber',
                     },
           );
}
       
#-----------------------------------------------------------------------------
# http://jabber.org/protocol/muc
#-----------------------------------------------------------------------------
{
    &add_ns(ns    => 'http://jabber.org/protocol/muc',
            tag   => 'x',
            xpath => {
                      Password => { path => 'password/text()' },
                      MUC      => { type => 'master' },
                     },
            docs  => {
                      module => 'Net::Jabber',
                     },
           );
}
       
#-----------------------------------------------------------------------------
# http://jabber.org/protocol/muc#admin
#-----------------------------------------------------------------------------
{
    &add_ns(ns    => 'http://jabber.org/protocol/muc#admin',
            tag   => 'query',
            xpath => {
                      Item  => {
                                type  => 'child',
                                path  => 'item',
                                child => { ns => '__netjabber__:iq:muc:admin:item' },
                                calls => [ 'Add' ],
                               },
                      Items => {
                                type  => 'child',
                                path  => 'item',
                                child => { ns => '__netjabber__:iq:muc:admin:item' },
                               },
                      Admin => { type => 'master' },
                    },
            docs  => {
                      module => 'Net::Jabber',
                     },
           );
}

#-----------------------------------------------------------------------------
# __netjabber__:iq:muc:admin:item
#-----------------------------------------------------------------------------
{
    &add_ns(ns    => '__netjabber__:iq:muc:admin:item',
            xpath => {
                      ActorJID    => {
                                      type => 'jid',
                                      path => 'actor/@jid',
                                     },
                      Affiliation => { path => '@affiliation' },
                      JID         => {
                                      type => 'jid',
                                      path => '@jid',
                                     },
                      Nick        => { path => '@nick' },
                      Reason      => { path => 'reason/text()' },
                      Role        => { path => '@role' },
                      Item        => { type => 'master' },
                     },
            docs  => {
                      module => 'Net::Jabber',
                      name   => 'http://jabber.org/protocol/muc#admin - item objects',
                     },
           );
}

#-----------------------------------------------------------------------------
# http://jabber.org/protocol/muc#user
#-----------------------------------------------------------------------------
{
    &add_ns(ns    => 'http://jabber.org/protocol/muc#user',
            tag   => 'x',
            xpath => {
                      Alt        => { path => 'alt/text()' },
                      Invite     => {
                                     type  => 'child',
                                     path  => 'invite',
                                     child => { ns => '__netjabber__:x:muc:invite' },
                                     calls => [ 'Get', 'Defined', 'Add', 'Remove' ],
                                    },
                      Item       => {
                                     type  => 'child',
                                     path  => 'item',
                                     child => { ns => '__netjabber__:x:muc:item' },
                                     calls => [ 'Get', 'Defined', 'Add', 'Remove' ],
                                    },
                      Password   => { path => 'password/text()' },
                      StatusCode => { path => 'status/@code' },
                      User       => { type => 'master' },
                     },
            docs  => {
                      module => 'Net::Jabber',
                     },
           );
}

#-----------------------------------------------------------------------------
# __netjabber__:x:muc:invite
#-----------------------------------------------------------------------------
{
    &add_ns(ns    => '__netjabber__:x:muc:invite',
            xpath => {
                      From   => {
                                 type => 'jid',
                                 path => '@from',
                                },
                      Reason => { path => 'reason/text()' },
                      To     => {
                                 type => 'jid',
                                 path => '@to',
                                },
                      Invite => { type => 'master' },
                     },
            docs  => {
                      module => 'Net::Jabber',
                      name   => 'http://jabber.org/protocol/muc#user - invite objects',
                     },
           );
}

#-----------------------------------------------------------------------------
# __netjabber__:x:muc:item
#-----------------------------------------------------------------------------
{
    &add_ns(ns    => '__netjabber__:x:muc:item',
            xpath => {
                      ActorJID    => {
                                      type => 'jid',
                                      path => 'actor/@jid',
                                     },
                      Affiliation => { path => '@affiliation' },
                      JID         => {
                                      type => 'jid',
                                      path => '@jid',
                                     },
                      Nick        => { path => '@nick' },
                      Reason      => { path => 'reason/text()' },
                      Role        => { path => '@role' },
                      Item        => { type => 'master' },
                     },
            docs  => {
                      module => 'Net::Jabber',
                      name   => 'http://jabber.org/protocol/muc#user - item objects',
                     },
           );
}

#-----------------------------------------------------------------------------
# http://jabber.org/protocol/pubsub
#-----------------------------------------------------------------------------
{
    &add_ns(ns    => 'http://jabber.org/protocol/pubsub',
            tag   => 'pubsub',
            xpath => {
                      Affiliations => {
                                       type  => 'child',
                                       path  => 'affiliations',
                                       child => { ns => '__netjabber__:iq:pubsub:affiliations' },
                                       calls => [ 'Get', 'Defined', 'Add', 'Remove' ],
                                      },
                      Configure    => {
                                       type  => 'child',
                                       path  => 'configure',
                                       child => { ns => '__netjabber__:iq:pubsub:configure' },
                                       calls => [ 'Get', 'Defined', 'Add', 'Remove' ],
                                      },
                      Create       => {
                                       type  => 'child',
                                       path  => 'create',
                                       child => { ns => '__netjabber__:iq:pubsub:create' },
                                       calls => [ 'Get', 'Defined', 'Add', 'Remove' ],
                                      },
                      Delete       => {
                                       type  => 'child',
                                       path  => 'delete',
                                       child => { ns => '__netjabber__:iq:pubsub:delete' },
                                       calls => [ 'Get', 'Defined', 'Add', 'Remove' ],
                                      },
                      Entities     => {
                                       type  => 'child',
                                       path  => 'entities',
                                       child => { ns => '__netjabber__:iq:pubsub:entities' },
                                       calls => [ 'Get', 'Defined', 'Add', 'Remove' ],
                                      },
                      Entity       => {
                                       type  => 'child',
                                       path  => 'entity',
                                       child => { ns => '__netjabber__:iq:pubsub:entity' },
                                       calls => [ 'Get', 'Defined', 'Add', 'Remove' ],
                                      },
                      Items        => {
                                       type  => 'child',
                                       path  => 'items',
                                       child => { ns => '__netjabber__:iq:pubsub:items' },
                                       calls => [ 'Get', 'Defined', 'Add', 'Remove' ],
                                      },
                      Item         => {
                                       type  => 'child',
                                       path  => 'item',
                                       child => { ns => '__netjabber__:iq:pubsub:item' },
                                       calls => [ 'Get', 'Defined', 'Add', 'Remove' ],
                                      },
                      Options      => {
                                       type  => 'child',
                                       path  => 'options',
                                       child => { ns => '__netjabber__:iq:pubsub:options' },
                                       calls => [ 'Get', 'Defined', 'Add', 'Remove' ],
                                      },
                      Publish      => {
                                       type  => 'child',
                                       path  => 'publish',
                                       child => { ns => '__netjabber__:iq:pubsub:publish' },
                                       calls => [ 'Get', 'Defined', 'Add', 'Remove' ],
                                      },
                      Purge        => {
                                       type  => 'child',
                                       path  => 'purge',
                                       child => { ns => '__netjabber__:iq:pubsub:purge' },
                                       calls => [ 'Get', 'Defined', 'Add', 'Remove' ],
                                      },
                      Retract      => {
                                       type  => 'child',
                                       path  => 'retract',
                                       child => { ns => '__netjabber__:iq:pubsub:retract' },
                                       calls => [ 'Get', 'Defined', 'Add', 'Remove' ],
                                      },
                      Subscribe    => {
                                       type  => 'child',
                                       path  => 'subscribe',
                                       child => { ns => '__netjabber__:iq:pubsub:subscribe' },
                                       calls => [ 'Get', 'Defined', 'Add', 'Remove' ],
                                      },
                      Unsubscribe  => {
                                       type  => 'child',
                                       path  => 'unsubscribe',
                                       child => { ns => '__netjabber__:iq:pubsub:unsubscribe' },
                                       calls => [ 'Get', 'Defined', 'Add', 'Remove' ],
                                      },
                      PubSub       => { type => 'master' },
                     },
            docs  => {
                      module => 'Net::Jabber',
                     },
           );
}

#-----------------------------------------------------------------------------
# __netjabber__:iq:pubsub:affiliations
#-----------------------------------------------------------------------------
{
    &add_ns(ns    => '__netjabber__:iq:pubsub:affiliations',
            xpath => {
                      Entity       => {
                                       type  => 'child',
                                       path  => 'entity',
                                       child => { ns => '__netjabber__:iq:pubsub:entity' },
                                       calls => [ 'Get', 'Defined', 'Add', 'Remove' ],
                                      },
                      Affiliations => { type => 'master' },
                     },
            docs  => {
                      module => 'Net::Jabber',
                      name   => 'http://jabber.org/protocol/pubsub - affiliations objects',
                     },
           );
}

#-----------------------------------------------------------------------------
# __netjabber__:iq:pubsub:configure
#-----------------------------------------------------------------------------
{
    &add_ns(ns    => '__netjabber__:iq:pubsub:configure',
            xpath => {
                      Node      => { path => '@node' },
                      Configure => { type => 'master' },
                     },
            docs  => {
                      module => 'Net::Jabber',
                      name   => 'http://jabber.org/protocol/pubsub - configure objects',
                     },
           );
}

#-----------------------------------------------------------------------------
# __netjabber__:iq:pubsub:create
#-----------------------------------------------------------------------------
{
    &add_ns(ns    => '__netjabber__:iq:pubsub:create',
            xpath => {
                      Node   => { path => '@node' },
                      Create => { type => 'master' },
                     },
            docs  => {
                      module => 'Net::Jabber',
                      name   => 'http://jabber.org/protocol/pubsub - create objects',
                     },
           );
}

#-----------------------------------------------------------------------------
# __netjabber__:iq:pubsub:delete
#-----------------------------------------------------------------------------
{
    &add_ns(ns    => '__netjabber__:iq:pubsub:delete',
            xpath => {
                      Node   => { path => '@node' },
                      Delete => { type => 'master' },
                     },
            docs  => {
                      module => 'Net::Jabber',
                      name   => 'http://jabber.org/protocol/pubsub - delete objects',
                     },
           );
}

#-----------------------------------------------------------------------------
# __netjabber__:iq:pubsub:entities
#-----------------------------------------------------------------------------
{
    &add_ns(ns    => '__netjabber__:iq:pubsub:entities',
            xpath => {
                      Entity   => {
                                   type  => 'child',
                                   path  => 'entity',
                                   child => { ns => '__netjabber__:iq:pubsub:entity' },
                                   calls => [ 'Get', 'Defined', 'Add', 'Remove' ],
                                  },
                      Entities => { type => 'master' },
                     },
            docs  => {
                      module => 'Net::Jabber',
                      name   => 'http://jabber.org/protocol/pubsub - entities objects',
                     },
           );
}

#-----------------------------------------------------------------------------
# __netjabber__:iq:pubsub:entity
#-----------------------------------------------------------------------------
{
    &add_ns(ns    => '__netjabber__:iq:pubsub:entity',
            xpath => {
                      Affiliation      => { path => '@affiliation' },
                      JID              => {
                                           type => 'jid',
                                           path => '@jid',
                                          },
                      Node             => { path => '@node' },
                      Subscription     => { path => '@subscription' },
                      SubscribeOptions => {
                                           type  => 'child',
                                           path  => 'subscribe-options',
                                           child => { ns => '__netjabber__:iq:pubsub:subscribe-options' },
                                           calls => [ 'Get', 'Defined', 'Add', 'Remove' ],
                                          },
                      Entity           => { type => 'master' },
                     },
            docs  => {
                      module => 'Net::Jabber',
                      name   => 'http://jabber.org/protocol/pubsub - entity objects',
                     },
           );
}

#-----------------------------------------------------------------------------
# __netjabber__:iq:pubsub:items
#-----------------------------------------------------------------------------
{
    &add_ns(ns    => '__netjabber__:iq:pubsub:items',
            xpath => {
                      Item     => {
                                   type  => 'child',
                                   path  => 'item',
                                   child => { ns => '__netjabber__:iq:pubsub:item' },
                                   calls => [ 'Get', 'Defined', 'Add', 'Remove' ],
                                  },
                      Node     => { path => '@node' },
                      MaxItems => { path => '@max_items' },
                      Items    => { type => 'master' },
                     },
            docs  => {
                      module => 'Net::Jabber',
                      name   => 'http://jabber.org/protocol/pubsub - items objects',
                     },
           );
}

#-----------------------------------------------------------------------------
# __netjabber__:iq:pubsub:item
#-----------------------------------------------------------------------------
{
    &add_ns(ns    => '__netjabber__:iq:pubsub:item',
            xpath => {
                      ID      => { path => '@id' },
                      Payload => { type => 'raw' },
                      Item    => { type => 'master' },
                     },
            docs  => {
                      module => 'Net::Jabber',
                      name   => 'http://jabber.org/protocol/pubsub - item objects',
                     },
           );
}

#-----------------------------------------------------------------------------
# __netjabber__:iq:pubsub:options
#-----------------------------------------------------------------------------
{
    &add_ns(ns    => '__netjabber__:iq:pubsub:options',
            xpath => {
                      JID     => {
                                  type => 'jid',
                                  path => '@jid',
                                 },
                      Node    => { path => '@node' },
                      Options => { type => 'master' },
                     },
            docs  => {
                      module => 'Net::Jabber',
                      name   => 'http://jabber.org/protocol/pubsub - options objects',
                     },
           );
}

#-----------------------------------------------------------------------------
# __netjabber__:iq:pubsub:publish
#-----------------------------------------------------------------------------
{
    &add_ns(ns    => '__netjabber__:iq:pubsub:publish',
            xpath => {
                      Item    => {
                                  type  => 'child',
                                  path  => 'item',
                                  child => { ns => '__netjabber__:iq:pubsub:item' },
                                  calls => [ 'Get', 'Defined', 'Add', 'Remove' ],
                                 },
                      Node    => { path => '@node' },
                      Publish => { type => 'master' },
                     },
            docs  => {
                      module => 'Net::Jabber',
                      name   => 'http://jabber.org/protocol/pubsub - publish objects',
                     },
           );
}

#-----------------------------------------------------------------------------
# __netjabber__:iq:pubsub:purge
#-----------------------------------------------------------------------------
{
    &add_ns(ns    => '__netjabber__:iq:pubsub:purge',
            xpath => {
                      Node  => { path => '@node' },
                      Purge => { type => 'master' },
                     },
            docs  => {
                      module => 'Net::Jabber',
                      name   => 'http://jabber.org/protocol/pubsub - purge objects',
                     },
           );
}

#-----------------------------------------------------------------------------
# __netjabber__:iq:pubsub:retract
#-----------------------------------------------------------------------------
{
    &add_ns(ns    => '__netjabber__:iq:pubsub:retract',
            xpath => {
                      Item    => {
                                  type  => 'child',
                                  path  => 'item',
                                  child => { ns => '__netjabber__:iq:pubsub:item' },
                                  calls => [ 'Get', 'Defined', 'Add', 'Remove' ],
                                 },
                      Node    => { path => '@node' },
                      Retract => { type => 'master' },
                     },
            docs  => {
                      module => 'Net::Jabber',
                      name   => 'http://jabber.org/protocol/pubsub - retract objects',
                     },
           );
}

#-----------------------------------------------------------------------------
# __netjabber__:iq:pubsub:subscribe
#-----------------------------------------------------------------------------
{
    &add_ns(ns    => '__netjabber__:iq:pubsub:subscribe',
            xpath => {
                      JID       => {
                                    type => 'jid',
                                    path => '@jid',
                                   },
                      Node      => { path => '@node' },
                      Subscribe => { type => 'master' },
                     },
            docs  => {
                      module => 'Net::Jabber',
                      name   => 'http://jabber.org/protocol/pubsub - subscribe objects',
                     },
           );
}

#-----------------------------------------------------------------------------
# __netjabber__:iq:pubsub:subscribe-options
#-----------------------------------------------------------------------------
{
    &add_ns(ns    => '__netjabber__:iq:pubsub:subscribe-options',
            xpath => {
                      Required         => {
                                           type => 'flag',
                                           path => 'required',
                                          },
                      SubscribeOptions => { type => 'master' },
                     },
            docs  => {
                      module => 'Net::Jabber',
                      name   => 'http://jabber.org/protocol/pubsub - subscribe-options objects',
                     },
           );
}

#-----------------------------------------------------------------------------
# __netjabber__:iq:pubsub:unsubscribe
#-----------------------------------------------------------------------------
{
    &add_ns(ns    => '__netjabber__:iq:pubsub:unsubscribe',
            xpath => {
                      JID         => {
                                      type => 'jid',
                                      path => '@jid',
                                     },
                      Node        => { path => '@node' },
                      Unsubscribe => { type => 'master' },
                     },
            docs  => {
                      module => 'Net::Jabber',
                      name   => 'http://jabber.org/protocol/pubsub - unsubscribe objects',
                     },
           );
}

#-----------------------------------------------------------------------------
# http://jabber.org/protocol/pubsub#event
#-----------------------------------------------------------------------------
{
    &add_ns(ns    => 'http://jabber.org/protocol/pubsub#event',
            tag   => 'event',
            xpath => {
                      Delete => {
                                 type  => 'child',
                                 path  => 'delete',
                                 child => { ns => '__netjabber__:iq:pubsub:event:delete' },
                                 calls => [ 'Get', 'Defined', 'Add', 'Remove' ],
                                },
                      Items  => {
                                 type  => 'child',
                                 path  => 'items',
                                 child => { ns => '__netjabber__:iq:pubsub:event:items' },
                                 calls => [ 'Get', 'Defined', 'Add', 'Remove' ],
                                },
                      Event  => { type => 'master' },
                     },
            docs  => {
                      module => 'Net::Jabber',
                     },
           );
}

#-----------------------------------------------------------------------------
# __netjabber__:iq:pubsub:event:delete
#-----------------------------------------------------------------------------
{
    &add_ns(ns    => '__netjabber__:iq:pubsub:event:delete',
            xpath => {
                      Node   => { path => '@node' },
                      Delete => { type => 'master' },
                     },
            docs  => {
                      module => 'Net::Jabber',
                      name   => 'http://jabber.org/protocol/pubsub#event - delete objects',
                     },
           );
}

#-----------------------------------------------------------------------------
# __netjabber__:iq:pubsub:event:items
#-----------------------------------------------------------------------------
{
    &add_ns(ns    => '__netjabber__:iq:pubsub:event:items',
            xpath => {
                      Item  => {
                                type  => 'child',
                                path  => 'item',
                                child => { ns => '__netjabber__:iq:pubsub:event:item' },
                                calls => [ 'Get', 'Defined', 'Add', 'Remove' ],
                               },
                      Node  => { path => '@node' },
                      Items => { type => 'master' },
                     },
            docs  => {
                      module => 'Net::Jabber',
                      name   => 'http://jabber.org/protocol/pubsub#event - items objects',
                     },
           );
}

#-----------------------------------------------------------------------------
# __netjabber__:iq:pubsub:event:item
#-----------------------------------------------------------------------------
{
    &add_ns(ns    => '__netjabber__:iq:pubsub:event:item',
            xpath => {
                      ID      => { path => '@id' },
                      Payload => { type => 'raw' },
                      Item    => { type => 'master' },
                     },
            docs  => {
                      module => 'Net::Jabber',
                      name   => 'http://jabber.org/protocol/pubsub#event - item objects',
                     },
           );
}

#-----------------------------------------------------------------------------
# http://jabber.org/protocol/pubsub#owner
#-----------------------------------------------------------------------------
{
    &add_ns(ns    => 'http://jabber.org/protocol/pubsub#owner',
            tag   => 'pubsub',
            xpath => {
                      Action    => { path => '@action' },
                      Configure => {
                                    type  => 'child',
                                    path  => 'configure',
                                    child => { ns => '__netjabber__:iq:pubsub:owner:configure' },
                                    calls => [ 'Get', 'Defined', 'Add', 'Remove' ],
                                   },
                      Owner     => { type => 'master' },
                     },
            docs  => {
                      module => 'Net::Jabber',
                     },
           );
}

#-----------------------------------------------------------------------------
# __netjabber__:iq:pubsub:owner:configure
#-----------------------------------------------------------------------------
{
    &add_ns(ns    => '__netjabber__:iq:pubsub:owner:configure',
            xpath => {
                      Node      => { path => '@node' },
                      Configure => { type => 'master' },
                     },
            docs  => {
                      module => 'Net::Jabber',
                      name   => 'http://jabber.org/protocol/pubsub#owner - configure objects',
                     },
           );
}

#XXX pubsub#errors

#-----------------------------------------------------------------------------
# http://jabber.org/protocol/si
#-----------------------------------------------------------------------------
{
    &add_ns(ns    => 'http://jabber.org/protocol/si',
            tag   => 'si',
            xpath => {
                      ID       => { path => '@id' },
                      MimeType => { path => '@mime-type' },
                      Profile  => { path => '@profile' },
                      Stream   => { type => 'master' },
                     },
            docs  => {
                      module => 'Net::Jabber',
                     },
           );
}

#-----------------------------------------------------------------------------
# http://jabber.org/protocol/si/profile/file-transfer
#-----------------------------------------------------------------------------
{
    &add_ns(ns    => 'http://jabber.org/protocol/si/profile/file-transfer',
            tag   => 'file',
            xpath => {
                      Date        => { path => '@date' },
                      Desc        => { path => 'desc/text()' },
                      Hash        => { path => '@hash' },
                      Name        => { path => '@name' },
                      Range       => {
                                      type => 'flag',
                                      path => 'range',
                                     },
                      RangeOffset => { path => 'range/@offset' },
                      RangeLength => { path => 'range/@length' },
                      Size        => { path => '@size' },
                      File        => { type => 'master' },
                     },
            docs  => {
                      module => 'Net::Jabber',
                     },
           );
}

#-----------------------------------------------------------------------------
# http://jabber.org/protocol/si/profile/tree-transfer
#-----------------------------------------------------------------------------
# XXX do later
$ns = 'http://jabber.org/protocol/si/profile/tree-transfer';

$TAGS{$ns} = "tree";

$NAMESPACES{$ns}->{Directory}->{XPath}->{Type} = 'child';
$NAMESPACES{$ns}->{Directory}->{XPath}->{Path} = 'directory';
$NAMESPACES{$ns}->{Directory}->{XPath}->{Child} = ['Query','__netjabber__:iq:si:profile:tree:directory'];
$NAMESPACES{$ns}->{Directory}->{XPath}->{Calls} = ['Add','Get'];

$NAMESPACES{$ns}->{Numfiles}->{XPath}->{Path} = '@numfiles';

$NAMESPACES{$ns}->{Size}->{XPath}->{Path} = '@size';

$NAMESPACES{$ns}->{Tree}->{XPath}->{Type} = 'master';

#-----------------------------------------------------------------------------
# __netjabber__:iq:si:profile:tree:directory
#-----------------------------------------------------------------------------
$ns = '__netjabber__:iq:si:profile:tree:directory';

$NAMESPACES{$ns}->{Directory}->{XPath}->{Type} = 'child';
$NAMESPACES{$ns}->{Directory}->{XPath}->{Path} = 'directory';
$NAMESPACES{$ns}->{Directory}->{XPath}->{Child} = ['Query','__netjabber__:iq:si:profile:tree:directory'];
$NAMESPACES{$ns}->{Directory}->{XPath}->{Calls} = ['Add','Get'];

$NAMESPACES{$ns}->{File}->{XPath}->{Type} = 'child';
$NAMESPACES{$ns}->{File}->{XPath}->{Path} = 'file';
$NAMESPACES{$ns}->{File}->{XPath}->{Child} = ['Query','__netjabber__:iq:si:profile:tree:file'];
$NAMESPACES{$ns}->{File}->{XPath}->{Calls} = ['Add','Get'];

$NAMESPACES{$ns}->{Name}->{XPath}->{Path} = '@name';

$NAMESPACES{$ns}->{Dir}->{XPath}->{Type} = 'master';

#-----------------------------------------------------------------------------
# __netjabber__:iq:si:profile:tree:file
#-----------------------------------------------------------------------------
$ns = '__netjabber__:iq:si:profile:tree:file';

$NAMESPACES{$ns}->{Name}->{XPath}->{Path} = '@name';

$NAMESPACES{$ns}->{SID}->{XPath}->{Path} = '@sid';

$NAMESPACES{$ns}->{File}->{XPath}->{Type} = 'master';

sub add_ns
{
    &Net::XMPP::Namespaces::add_ns(@_);
}


1;
