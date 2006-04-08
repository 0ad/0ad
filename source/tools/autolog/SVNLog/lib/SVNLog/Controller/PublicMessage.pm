package SVNLog::Controller::PublicMessage;

use strict;
use base 'Catalyst::Base';

sub default : Private {
    my ( $self, $c ) = @_;
    $c->forward('list');
}

sub do_edit : Local {
    my ( $self, $c, $id ) = @_;
    $c->form(optional => [ SVNLog::Model::CDBI::PublicMessage->columns ],
        missing_optional_valid => 1 );
    if ($c->form->has_missing) {
        $c->stash->{message}='You have to fill in all fields.'.
        'the following are missing: <b>'.
        join(', ',$c->form->missing()).'</b>';
    } elsif ($c->form->has_invalid) {
        $c->stash->{message}='Some fields are not correctly filled in.'.
        'the following are invalid: <b>'.
    join(', ',$c->form->invalid()).'</b>';
    } else {
    SVNLog::Model::CDBI::PublicMessage->retrieve($id)->update_from_form( $c->form );
    $c->stash->{message}='Updated OK';
    }
    $c->forward('edit');
}

sub edit : Local {
    my ( $self, $c, $id ) = @_;
    $c->stash->{item} = SVNLog::Model::CDBI::PublicMessage->retrieve($id);
    $c->stash->{template} = 'PublicMessage/edit.tt';
}

sub list : Local {
    my ( $self, $c ) = @_;
    $c->stash->{template} = 'PublicMessage/list.tt';
}

1;
