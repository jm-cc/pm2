from spack import *
import platform

class Tbx(AutotoolsPackage):
    """TBX pm2 toolbox"""
    homepage = "http://pm2.gforge.inria.fr/"

    version('trunk', svn='https://scm.gforge.inria.fr/anonscm/svn/pm2/trunk/tbx')

    variant('debug', default=False, description='Build in debug mode')
    variant('optimize', default=True, description='Build in optimized mode')

    depends_on('pkgconfig')
    depends_on('autoconf')
    depends_on('libtool')
    depends_on('automake')
    
    def configure_args(self):
        spec = self.spec

        config_args = [
            '--disable-trace',
            ]
        
        config_args.extend([
            "--%s-debug"         % ('enable' if '+debug'     in spec else 'disable'),
            "--%s-optimize"      % ('enable' if '+optimize'  in spec else 'disable'),
        ])

        return config_args
    
        
